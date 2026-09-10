#!/usr/bin/env python3
"""HOST PDM capture units, adapter bind, and fail-closed current-target receipts.

Compiles platform/ra8p1/pdm_capture.c for the host. Does not open USB, serial,
flash, or a microphone. CROSS_COMPILED is compile-only. CURRENT_TARGET is
refused without live exclusive identity. Physical capture stays NOT_RUN.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from ctypes import CDLL, POINTER, c_int, c_int16, c_int32, c_uint32, c_size_t
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping

from verify_imports import ROOT

ADAPTER_ID = "k1-ra8p1-pdm-capture-v1"
LEVELS = ("HOST", "CROSS_COMPILED", "HISTORICAL_TARGET", "CURRENT_TARGET")
REQUIRED_PDM_FIELDS = (
    "requested_frames",
    "capture_bytes",
    "conversion_bytes",
    "submission_bytes",
    "capture_channels",
    "capture_element_bytes",
    "output_element_bytes",
    "first_data_does_not_prove_complete",
    "full_buffer_completion",
    "final_stopped_count",
    "stop_tail",
)
CURRENT_TARGET_IDENTITY = (
    "target_uid",
    "loaded_image_sha256",
    "build_id",
    "ownership_record.exclusive_owner",
    "ownership_record.method",
    "measurement_method_id",
    "input_identities",
    "runtime_identity_method",
)
CURRENT_TARGET_NUMERIC = (
    "full_buffer_completion",
    "final_stopped_count",
    "stop_tail",
    "output_duration_s",
)
PHYSICAL_BLOCKERS = (
    "re-enumeration of UID",
    "exact loaded-image identity",
    "exclusive ownership record",
)
LAST_IDENTIFIED_UID = "545433931bd25436593630352d068363"
LAST_IDENTIFIED_BUILD = "c4ceebe7f4d899d39a917fb12c385c0f743278fd53aa2a82045230f3490e87bb"
ARM_GCC = (
    ROOT.parent
    / "toolchains/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin/arm-none-eabi-gcc"
)
SCALAR_CFLAGS = (
    "-mcpu=cortex-m85",
    "-mthumb",
    "-mfpu=fpv5-sp-d16",
    "-mfloat-abi=hard",
    "-ffp-contract=off",
    "-fno-fast-math",
    "-fno-tree-vectorize",
    "-fno-tree-slp-vectorize",
)
PLUGIN_DIR = ROOT / "docs/evidence/K1-RA8P1-002/plugin-pdm"
SOURCE_C = ROOT / "platform/ra8p1/pdm_capture.c"
SOURCE_H = ROOT / "platform/ra8p1/pdm_capture.h"
ADAPTER_PATH = PLUGIN_DIR / "adapter-k1-ra8p1-pdm-capture-v1.json"

VENDOR_16000_SUBMISSION_BYTES = 32000
CORRECTED_UNITS = {
    (16000, 2): {"capture_bytes": 64000, "conversion_bytes": 64000, "submission_bytes": 64000, "callback_interval": 4000},
    (16000, 1): {"capture_bytes": 64000, "conversion_bytes": 32000, "submission_bytes": 32000, "callback_interval": 4000},
    (8000, 2): {"capture_bytes": 32000, "conversion_bytes": 32000, "submission_bytes": 32000, "callback_interval": 2000},
    (8000, 1): {"capture_bytes": 32000, "conversion_bytes": 16000, "submission_bytes": 16000, "callback_interval": 2000},
}


class PdmCaptureError(ValueError):
    def __init__(self, code: str, message: str) -> None:
        super().__init__(f"{code}: {message}")
        self.code = code
        self.message = message


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 16), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def observed_source_hashes() -> dict[str, str]:
    return {
        "application": sha256_file(SOURCE_C),
        "application_header": sha256_file(SOURCE_H),
    }


def load_adapter(path: Path = ADAPTER_PATH) -> dict[str, Any]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("id") != ADAPTER_ID:
        raise PdmCaptureError("adapter_id", f"Adapter id {payload.get('id')!r} is not {ADAPTER_ID}.")
    return payload


def bind_adapter(
    observed: Mapping[str, str] | None = None,
    adapter: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    adapter = adapter or load_adapter()
    observed = dict(observed or observed_source_hashes())
    expected = adapter.get("inputs") or {}
    missing: list[str] = []
    mismatched: list[str] = []
    matched: dict[str, str] = {}
    for name, spec in expected.items():
        want = spec.get("sha256") if isinstance(spec, Mapping) else None
        have = observed.get(name)
        if not want or have is None:
            missing.append(name)
            continue
        if have != want:
            mismatched.append(name)
        else:
            matched[name] = have
    ok = not missing and not mismatched
    return {
        "ok": ok,
        "adapter_id": adapter.get("id"),
        "matched": matched,
        "missing": missing,
        "mismatched": mismatched,
        "declined": None if ok else "unrecognised_source_hash",
    }


def derive_units(requested_frames: int, output_channels: int) -> dict[str, int]:
    key = (requested_frames, output_channels)
    if key not in CORRECTED_UNITS:
        raise PdmCaptureError("unsupported_fixture", f"No corrected ledger for frames={requested_frames} ch={output_channels}.")
    capture_channels = 1
    capture_element_bytes = 4
    output_element_bytes = 2
    callback_interval = requested_frames // 4
    capture_bytes = requested_frames * capture_channels * capture_element_bytes
    conversion_bytes = requested_frames * output_channels * output_element_bytes
    expected = CORRECTED_UNITS[key]
    if capture_bytes != expected["capture_bytes"] or conversion_bytes != expected["conversion_bytes"]:
        raise PdmCaptureError("ledger_mismatch", "Derived units do not match the corrected current-source ledger.")
    if conversion_bytes != expected["submission_bytes"]:
        raise PdmCaptureError("vendor_defect", "Corrected submission must equal conversion bytes.")
    return {
        "requested_frames": requested_frames,
        "capture_channels": capture_channels,
        "capture_element_bytes": capture_element_bytes,
        "output_channels": output_channels,
        "output_element_bytes": output_element_bytes,
        "callback_interval": callback_interval,
        "capture_bytes": capture_bytes,
        "conversion_bytes": conversion_bytes,
        "submission_bytes": conversion_bytes,
    }


def compile_host_library(output: Path) -> dict[str, Any]:
    command = [
        "cc",
        "-std=c11",
        "-shared",
        "-fPIC",
        "-O2",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I" + str(SOURCE_C.parent),
        str(SOURCE_C),
        "-o",
        str(output),
    ]
    completed = subprocess.run(command, check=False, capture_output=True, text=True)
    if completed.returncode != 0:
        raise PdmCaptureError(
            "host_compile_failed",
            (completed.stderr or completed.stdout or "host compile failed").strip(),
        )
    return {
        "command": command,
        "exit_code": completed.returncode,
        "output": str(output),
        "sha256": sha256_file(output),
        "qualification_level": "HOST",
        "compiler": subprocess.check_output(["cc", "--version"], text=True).splitlines()[0],
    }


class HostPdm:
    def __init__(self, library: CDLL) -> None:
        self.lib = library
        for name in (
            "k1_pdm_configure",
            "k1_pdm_start",
            "k1_pdm_on_data",
            "k1_pdm_request_stop",
            "k1_pdm_on_stopped",
            "k1_pdm_convert",
        ):
            getattr(library, name)
        library.k1_pdm_configure.argtypes = [c_uint32, c_uint32, c_uint32, c_uint32, c_uint32, c_uint32]
        library.k1_pdm_configure.restype = c_int
        library.k1_pdm_start.argtypes = []
        library.k1_pdm_start.restype = None
        library.k1_pdm_on_data.argtypes = [c_uint32]
        library.k1_pdm_on_data.restype = c_int
        library.k1_pdm_request_stop.argtypes = []
        library.k1_pdm_request_stop.restype = None
        library.k1_pdm_on_stopped.argtypes = [c_uint32]
        library.k1_pdm_on_stopped.restype = c_int
        library.k1_pdm_convert.argtypes = [POINTER(c_int32), c_size_t, POINTER(c_int16), c_size_t]
        library.k1_pdm_convert.restype = c_int
        for name in (
            "k1_pdm_requested_frames",
            "k1_pdm_capture_channels",
            "k1_pdm_capture_element_bytes",
            "k1_pdm_output_channels",
            "k1_pdm_output_element_bytes",
            "k1_pdm_callback_interval",
            "k1_pdm_capture_bytes",
            "k1_pdm_conversion_bytes",
            "k1_pdm_submission_bytes",
            "k1_pdm_received_elements",
            "k1_pdm_final_stopped_count",
        ):
            fn = getattr(library, name)
            fn.argtypes = []
            fn.restype = c_uint32
        for name in (
            "k1_pdm_configured",
            "k1_pdm_running",
            "k1_pdm_first_data_seen",
            "k1_pdm_first_data_does_not_prove_complete",
            "k1_pdm_full_buffer_complete",
            "k1_pdm_stop_requested",
            "k1_pdm_stopped",
            "k1_pdm_stop_tail_known",
            "k1_pdm_ownership_transferred",
        ):
            fn = getattr(library, name)
            fn.argtypes = []
            fn.restype = c_int

    def configure(self, requested_frames: int, output_channels: int) -> None:
        units = derive_units(requested_frames, output_channels)
        rc = self.lib.k1_pdm_configure(
            units["requested_frames"],
            units["capture_channels"],
            units["capture_element_bytes"],
            units["output_channels"],
            units["output_element_bytes"],
            units["callback_interval"],
        )
        if rc != 0:
            raise PdmCaptureError("configure_failed", f"k1_pdm_configure returned {rc}")
        self.lib.k1_pdm_start()

    def on_data(self, interval_elements: int) -> None:
        rc = self.lib.k1_pdm_on_data(interval_elements)
        if rc != 0:
            raise PdmCaptureError("on_data_failed", f"k1_pdm_on_data returned {rc}")

    def stop(self, driver_count: int) -> None:
        self.lib.k1_pdm_request_stop()
        rc = self.lib.k1_pdm_on_stopped(driver_count)
        if rc != 0:
            raise PdmCaptureError("stop_failed", f"k1_pdm_on_stopped returned {rc}")

    def snapshot(self) -> dict[str, Any]:
        complete = bool(self.lib.k1_pdm_full_buffer_complete())
        stopped = bool(self.lib.k1_pdm_stopped())
        return {
            "requested_frames": int(self.lib.k1_pdm_requested_frames()),
            "capture_channels": int(self.lib.k1_pdm_capture_channels()),
            "capture_element_bytes": int(self.lib.k1_pdm_capture_element_bytes()),
            "output_channels": int(self.lib.k1_pdm_output_channels()),
            "output_element_bytes": int(self.lib.k1_pdm_output_element_bytes()),
            "callback_interval": int(self.lib.k1_pdm_callback_interval()),
            "capture_bytes": int(self.lib.k1_pdm_capture_bytes()),
            "conversion_bytes": int(self.lib.k1_pdm_conversion_bytes()),
            "submission_bytes": int(self.lib.k1_pdm_submission_bytes()),
            "received_elements": int(self.lib.k1_pdm_received_elements()),
            "first_data_seen": bool(self.lib.k1_pdm_first_data_seen()),
            "first_data_does_not_prove_complete": bool(self.lib.k1_pdm_first_data_does_not_prove_complete()),
            "full_buffer_completion": complete,
            "final_stopped_count": int(self.lib.k1_pdm_final_stopped_count()) if stopped else None,
            "stop_tail": None,
            "stop_tail_known": bool(self.lib.k1_pdm_stop_tail_known()),
            "ownership_transferred": bool(self.lib.k1_pdm_ownership_transferred()),
            "stopped": stopped,
        }


def load_host_pdm(directory: Path | None = None) -> tuple[HostPdm, dict[str, Any]]:
    close = False
    if directory is None:
        directory = Path(tempfile.mkdtemp(prefix="k1-pdm-host-"))
        close = True
    library_path = directory / ("libpdm_capture.dylib" if sys.platform == "darwin" else "libpdm_capture.so")
    compile_receipt = compile_host_library(library_path)
    library = CDLL(str(library_path))
    if close:
        compile_receipt["temp_dir"] = str(directory)
    return HostPdm(library), compile_receipt


def run_host_fixture(pdm: HostPdm, requested_frames: int, output_channels: int) -> dict[str, Any]:
    units = derive_units(requested_frames, output_channels)
    pdm.configure(requested_frames, output_channels)
    interval = units["callback_interval"]
    pdm.on_data(interval)
    after_first = pdm.snapshot()
    if after_first["full_buffer_completion"] or after_first["ownership_transferred"]:
        raise PdmCaptureError("first_interval_as_completion", "First DATA interval was treated as complete.")
    if after_first["received_elements"] != interval:
        raise PdmCaptureError("first_interval_count", "First interval did not record callback_interval elements.")
    remaining = requested_frames - interval
    while remaining > 0:
        step = interval if remaining > interval else remaining
        pdm.on_data(step)
        remaining -= step
    after_full = pdm.snapshot()
    if not after_full["full_buffer_completion"]:
        raise PdmCaptureError("completion_missing", "Cumulative count did not reach requested frames.")
    if after_full["ownership_transferred"]:
        raise PdmCaptureError("ownership_before_stop", "Ownership transferred before stop.")
    pdm.stop(requested_frames)
    after_stop = pdm.snapshot()
    if after_stop["final_stopped_count"] != requested_frames:
        raise PdmCaptureError("final_count", "Final stopped count does not match requested frames.")
    if after_stop["stop_tail"] is not None or after_stop["stop_tail_known"]:
        raise PdmCaptureError("invented_stop_tail", "Host fixture invented a stop tail.")
    if after_stop["submission_bytes"] != after_stop["conversion_bytes"]:
        raise PdmCaptureError("vendor_defect", "Submission bytes copied the vendor half-length defect.")
    if after_stop["submission_bytes"] == VENDOR_16000_SUBMISSION_BYTES and requested_frames == 16000 and output_channels == 2:
        raise PdmCaptureError("vendor_defect", "16000 stereo submission of 32000 bytes is the vendor defect.")
    expected = CORRECTED_UNITS[(requested_frames, output_channels)]
    for field, value in expected.items():
        if after_stop[field] != value:
            raise PdmCaptureError("ledger_mismatch", f"{field}={after_stop[field]} expected {value}")
    return {
        "units": units,
        "after_first": after_first,
        "after_full": after_full,
        "after_stop": after_stop,
        "qualification_level": "HOST",
    }


def missing_current_target_fields(identities: Mapping[str, Any] | None) -> list[str]:
    identities = identities or {}
    missing: list[str] = []
    ownership = identities.get("ownership_record") if isinstance(identities.get("ownership_record"), Mapping) else {}
    values = {
        "target_uid": identities.get("target_uid"),
        "loaded_image_sha256": identities.get("loaded_image_sha256"),
        "build_id": identities.get("build_id"),
        "ownership_record.exclusive_owner": ownership.get("exclusive_owner") if isinstance(ownership, Mapping) else None,
        "ownership_record.method": ownership.get("method") if isinstance(ownership, Mapping) else None,
        "measurement_method_id": identities.get("measurement_method_id"),
        "input_identities": identities.get("input_identities"),
        "runtime_identity_method": identities.get("runtime_identity_method"),
    }
    for field in CURRENT_TARGET_IDENTITY:
        if not values.get(field):
            missing.append(field)
    method = values.get("ownership_record.method")
    if method == "port-name":
        missing.append("target_uid-not-port")
    return missing


def missing_current_target_numeric(pdm: Mapping[str, Any] | None) -> list[str]:
    pdm = pdm or {}
    missing: list[str] = []
    for field in CURRENT_TARGET_NUMERIC:
        value = pdm.get(field)
        if value is None or value == "unknown":
            missing.append(field)
    return missing


def make_receipt(
    *,
    qualification_level: str,
    pdm: Mapping[str, Any],
    identities: Mapping[str, Any] | None = None,
    execution: Mapping[str, Any] | None = None,
    tools: list[dict[str, Any]] | None = None,
    limitations: list[str] | None = None,
    receipt_id: str = "wp13-pdm",
) -> dict[str, Any]:
    if qualification_level not in LEVELS:
        raise PdmCaptureError("qualification_level", f"Unknown qualification_level {qualification_level}.")
    identities = dict(identities or {})
    execution = dict(execution or {})
    errors: list[dict[str, str]] = []
    blocked: list[str] = []
    pdm_payload = dict(pdm)
    for field in REQUIRED_PDM_FIELDS:
        if field not in pdm_payload:
            errors.append({"code": "pdm-field-missing", "message": f"PDM field {field} is required"})
    if pdm_payload.get("first_data_does_not_prove_complete") is False:
        errors.append(
            {
                "code": "pdm-first-interval-as-completion",
                "message": "First DATA/interval cannot prove full-buffer completion",
            }
        )
    if pdm_payload.get("first_interval_proves_completion") is True:
        errors.append(
            {
                "code": "pdm-first-interval-as-completion",
                "message": "first_interval_proves_completion is forbidden",
            }
        )
    if qualification_level == "CURRENT_TARGET":
        for field in missing_current_target_fields(identities):
            errors.append({"code": "current-target-identity-missing", "message": field})
        for field in missing_current_target_numeric(pdm_payload):
            errors.append({"code": "pdm-current-target-incomplete", "message": field})
        if execution.get("compiler_facts_only"):
            errors.append(
                {
                    "code": "compiler-facts-as-current-target",
                    "message": "Build-only/compiler facts cannot claim CURRENT_TARGET completion",
                }
            )
        ownership = identities.get("ownership_record")
        if isinstance(ownership, Mapping) and ownership.get("method") == "port-name":
            errors.append({"code": "port-name-is-not-identity", "message": "USB/serial port names are not board identity"})
    if qualification_level != "CURRENT_TARGET" or errors:
        blocked.append("pdm-current-target")
    status = "PASS" if not errors else "FAIL"
    if qualification_level == "CURRENT_TARGET" and errors:
        status = "FAIL_CLOSED"
    physical = "NOT_RUN" if "pdm-current-target" in blocked else status
    return {
        "schema_version": 2,
        "receipt_id": receipt_id,
        "recipe": {"id": "pdm-capture", "version": "1"},
        "contract_version": "RA8P1-SOL-1.0.0",
        "qualification_level": qualification_level,
        "execution": execution,
        "identities": identities,
        "pdm": pdm_payload,
        "tools": tools or [],
        "errors": errors,
        "blocked_cells": blocked,
        "physical_capture": physical,
        "ok": not errors,
        "status": status,
        "limitations": limitations
        or [
            "HOST/CROSS_COMPILED work does not measure a microphone.",
            "Stop tail is not invented.",
            "Qualification remains engineering-preview.",
        ],
        "started_at": datetime.now(timezone.utc).isoformat(),
        "completed_at": datetime.now(timezone.utc).isoformat(),
    }


def physical_capture_not_run() -> dict[str, Any]:
    return make_receipt(
        qualification_level="HOST",
        receipt_id="wp13-pdm-physical-not-run",
        identities={
            "last_identified_uid": LAST_IDENTIFIED_UID,
            "last_identified_build_id": LAST_IDENTIFIED_BUILD,
            "last_identified_level": "HISTORICAL_TARGET",
            "live_target": "NOT_VERIFIED",
            "live_loaded_image": "unknown-no-reenumeration",
        },
        execution={"arm_binary_executed": False, "hardware_session": False},
        pdm={
            "requested_frames": 16000,
            "capture_bytes": 64000,
            "conversion_bytes": 64000,
            "submission_bytes": 64000,
            "capture_channels": 1,
            "capture_element_bytes": 4,
            "output_element_bytes": 2,
            "first_data_does_not_prove_complete": True,
            "full_buffer_completion": "unknown",
            "final_stopped_count": None,
            "stop_tail": None,
            "output_duration_s": None,
            "captured_signal": None,
        },
        limitations=[
            "live_target is NOT_VERIFIED; last UID is HISTORICAL_TARGET.",
            "Physical PDM cell is NOT_RUN, not a host-pass.",
            "Blocked until: " + "; ".join(PHYSICAL_BLOCKERS) + ".",
            "No USB, flash, serial, audio, cadence runner, or song loop.",
        ],
    )


def cross_compile_pdm_capture(output: Path) -> dict[str, Any]:
    if not ARM_GCC.is_file():
        return {
            "qualification_level": "CROSS_COMPILED",
            "status": "NOT_RUN",
            "missing_tool": str(ARM_GCC),
            "ok": False,
            "physical_capture": "NOT_RUN",
            "limitations": [f"Arm GNU 13.3.1 gcc is not at {ARM_GCC}."],
        }
    command = [
        str(ARM_GCC),
        "-std=c11",
        "-c",
        *SCALAR_CFLAGS,
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I" + str(SOURCE_C.parent),
        str(SOURCE_C),
        "-o",
        str(output),
    ]
    completed = subprocess.run(command, check=False, capture_output=True, text=True)
    compiler = subprocess.check_output([str(ARM_GCC), "--version"], text=True).splitlines()[0]
    if completed.returncode != 0:
        return {
            "qualification_level": "CROSS_COMPILED",
            "status": "FAIL",
            "ok": False,
            "command": command,
            "exit_code": completed.returncode,
            "stderr": completed.stderr,
            "compiler": compiler,
            "physical_capture": "NOT_RUN",
        }
    return {
        "qualification_level": "CROSS_COMPILED",
        "status": "PASS",
        "ok": True,
        "command": command,
        "exit_code": 0,
        "compiler": compiler,
        "compiler_path": str(ARM_GCC),
        "object": str(output),
        "object_sha256": sha256_file(output),
        "source_sha256": sha256_file(SOURCE_C),
        "header_sha256": sha256_file(SOURCE_H),
        "executed": False,
        "linked": False,
        "physical_capture": "NOT_RUN",
        "limitations": [
            "Compile-only. The object was not linked or executed.",
            "CROSS_COMPILED is not CURRENT_TARGET and not a microphone measurement.",
        ],
    }


def write_json(path: Path, payload: Mapping[str, Any]) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    blob = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    path.write_text(blob, encoding="utf-8")
    return sha256_bytes(blob.encode("utf-8"))


def build_adapter_document() -> dict[str, Any]:
    hashes = observed_source_hashes()
    return {
        "id": ADAPTER_ID,
        "version": "1",
        "adapter_class": "pdm_cpu_isr",
        "instance": "g_pdm0",
        "qualification_level": "HOST",
        "note": (
            "Current-source corrected PDM completion/units in the RA8P1 firmware "
            "repository. Not the vendor Titan_Mini_pdm snapshot. Not CURRENT_TARGET."
        ),
        "do_not_copy_vendor_defect": True,
        "inputs": {
            "application": {
                "path": "platform/ra8p1/pdm_capture.c",
                "sha256": hashes["application"],
            },
            "application_header": {
                "path": "platform/ra8p1/pdm_capture.h",
                "sha256": hashes["application_header"],
            },
        },
        "flow": {
            "producer": "cortex-m85-isr",
            "dmac_enabled": False,
            "active_data_callback": True,
            "events": {
                "PDM_EVENT_DATA": {
                    "active": True,
                    "actions": ["accumulate_received_elements"],
                    "proves_full_buffer": False,
                }
            },
            "stop": {
                "final_count_retained": True,
                "idle_callback": False,
            },
            "conversion": {
                "duplicate_mono_to_stereo_int16": True,
                "submission_equals_conversion": True,
            },
        },
        "expressions": {
            "capture_bytes": "requested_frames * capture_channels * capture_element_bytes",
            "conversion_bytes": "requested_frames * output_channels * output_element_bytes",
            "submission_bytes": "conversion_bytes",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=PLUGIN_DIR)
    args = parser.parse_args()
    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    adapter = build_adapter_document()
    write_json(output / "adapter-k1-ra8p1-pdm-capture-v1.json", adapter)
    bind = bind_adapter(adapter=adapter)
    if not bind["ok"]:
        raise SystemExit(json.dumps(bind))
    pdm, host_compile = load_host_pdm()
    fixtures = {}
    for frames, channels, name in (
        (16000, 2, "16000-stereo"),
        (16000, 1, "16000-mono"),
        (8000, 2, "8000-stereo"),
        (8000, 1, "8000-mono"),
    ):
        fixtures[name] = run_host_fixture(pdm, frames, channels)
    host_receipt = make_receipt(
        qualification_level="HOST",
        receipt_id="wp13-pdm-host-fixtures",
        identities={"adapter_id": ADAPTER_ID, "firmware_head": subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip()},
        execution={"arm_binary_executed": False, "compiler_facts_only": False, "host_library": True},
        pdm={
            **fixtures["16000-stereo"]["after_stop"],
            "fixtures": {name: item["after_stop"] for name, item in fixtures.items()},
            "first_data_does_not_prove_complete": True,
            "stop_tail": None,
        },
        tools=[{"name": "cc", "argv": host_compile["command"], "exit_code": 0, "sha256": host_compile["sha256"]}],
    )
    write_json(output / "host-receipt.json", host_receipt)
    with tempfile.TemporaryDirectory(prefix="k1-pdm-cross-") as temp:
        cross = cross_compile_pdm_capture(Path(temp) / "pdm_capture.o")
        if cross.get("object"):
            object_copy = output / "pdm_capture.cross.o"
            object_copy.write_bytes(Path(cross["object"]).read_bytes())
            cross["object"] = str(object_copy)
            cross["object_sha256"] = sha256_file(object_copy)
        write_json(output / "cross-compiled-receipt.json", cross)
    physical = physical_capture_not_run()
    write_json(output / "physical-capture-not-run.json", physical)
    print(json.dumps({"host": host_receipt["status"], "cross": cross.get("status"), "physical": physical["physical_capture"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
