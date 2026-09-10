#!/usr/bin/env python3
"""Useful-U55 admission: load-only refused, G6 preserved, goldens not invented.

Inspects the pinned Titan BSP face-detector tree for original same-compilation
model/goldens/allocation. Does not open USB, serial, flash, or audio. Does not
pick a different student. Licence UNKNOWN stays UNKNOWN.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from ctypes import CDLL, POINTER, c_char_p, c_int, c_uint32, c_uint8
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping

from verify_imports import ROOT

ADAPTER_ID = "k1-ra8p1-useful-npu-v1"
LEVELS = ("HOST", "CROSS_COMPILED", "HISTORICAL_TARGET", "CURRENT_TARGET")
SOURCE_C = ROOT / "platform/ra8p1/useful_npu_probe.c"
PLUGIN_DIR = ROOT / "docs/evidence/K1-RA8P1-002/plugin-compute"
BSP_ROOT = ROOT.parent / "sdk-bsp-ra8p1-titan-mini"
FACE_DETECT = BSP_ROOT / "project/Titan_Mini_npu_ai_face_detection"
BSP_COMMIT = "6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7"
ARM_GCC = (
    ROOT.parent
    / "toolchains/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin/arm-none-eabi-gcc"
)
SCALAR_CFLAGS = (
    "-mcpu=cortex-m85+nomve",
    "-mthumb",
    "-mfpu=fpv5-sp-d16",
    "-mfloat-abi=hard",
    "-ffp-contract=off",
    "-fno-fast-math",
    "-fno-tree-vectorize",
    "-fno-tree-slp-vectorize",
)
LAST_IDENTIFIED_UID = "545433931bd25436593630352d068363"
LAST_IDENTIFIED_BUILD = "c4ceebe7f4d899d39a917fb12c385c0f743278fd53aa2a82045230f3490e87bb"
LOAD_ONLY_TFLITE_SHA256 = "1fbdbb2878f5223697f70366036bf6260026f2e7ebbdb0dd0e0987d726f96fed"
LOAD_ONLY_ONNX_SHA256 = "5d0097e83fe269acb6ce92c5ab84cfe897561d3858328f2f0c610262c3801bd6"
LOAD_ONLY_OUTPUT = [127, 117, 123]
G6 = "NO_QUALIFYING_CANDIDATE"
DECLINE = {
    0: "OK",
    1: "LOAD_ONLY",
    2: "CONSTANT_OUTPUT",
    3: "MISSING_MODEL",
    4: "MISSING_GOLDENS",
    5: "MISSING_ALLOC",
    6: "MISSING_OPERATOR",
    7: "MISSING_PERMISSIONS",
    8: "LICENCE_UNKNOWN",
    9: "NOT_INPUT_DEPENDENT",
    10: "DIFFERENT_STUDENT",
}
GENERATED_C = (
    "src/models/sub_0000_invoke.c",
    "src/models/sub_0000_model_data.c",
    "src/models/sub_0000_tensors.c",
    "src/models/sub_0000_command_stream.c",
    "src/models/model.h",
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


class UsefulNpuError(ValueError):
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


def digest_bytes(hex_digest: str) -> bytes:
    return bytes.fromhex(hex_digest)


def as_c_digest(hex_digest: str | None):
    buf = (c_uint8 * 32)()
    if hex_digest:
        raw = digest_bytes(hex_digest)
        for i, value in enumerate(raw):
            buf[i] = value
    return buf


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
        raise UsefulNpuError(
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


class HostUsefulNpu:
    def __init__(self, library: CDLL) -> None:
        self.lib = library
        library.k1_useful_npu_admit.argtypes = [
            c_int, c_int, c_int, c_int, c_int, c_int, c_int, c_int, c_int,
            c_int, c_int, c_int, c_int,
            POINTER(c_uint8), POINTER(c_uint8), POINTER(c_uint8),
            POINTER(c_uint8), c_uint32,
        ]
        library.k1_useful_npu_admit.restype = c_int
        library.k1_useful_npu_probe.argtypes = []
        library.k1_useful_npu_probe.restype = c_int
        library.k1_useful_npu_invoke.argtypes = []
        library.k1_useful_npu_invoke.restype = c_int
        library.k1_useful_npu_admitted.restype = c_int
        library.k1_useful_npu_last_decline.restype = c_int
        library.k1_useful_npu_g6_no_qualifying.restype = c_int
        library.k1_useful_npu_licence_unknown.restype = c_int
        library.k1_useful_npu_invoked.restype = c_int
        library.k1_useful_npu_identity_set.restype = c_int
        library.k1_useful_npu_bind_identity.argtypes = [
            POINTER(c_uint8), POINTER(c_uint8), POINTER(c_uint8), POINTER(c_uint8)
        ]
        library.k1_useful_npu_identity_equal.argtypes = [POINTER(c_uint8), POINTER(c_uint8)]
        library.k1_useful_npu_identity_equal.restype = c_int
        library.k1_useful_npu_is_load_only_output.argtypes = [POINTER(c_uint8), c_uint32]
        library.k1_useful_npu_is_load_only_output.restype = c_int
        library.k1_useful_npu_copy_identity.argtypes = [POINTER(c_uint8)]
        library.k1_useful_npu_g6_text.restype = c_char_p

    def admit(self, **flags: Any) -> int:
        expected = flags.get("expected_raw") or []
        expected_buf = (c_uint8 * max(len(expected), 1))(*expected)
        return int(
            self.lib.k1_useful_npu_admit(
                int(flags.get("has_same_compilation_model", 0)),
                int(flags.get("has_allocation_report", 0)),
                int(flags.get("has_operator_report", 0)),
                int(flags.get("has_golden_inputs", 0)),
                int(flags.get("has_golden_outputs", 0)),
                int(flags.get("has_code_perm", 0)),
                int(flags.get("has_weight_perm", 0)),
                int(flags.get("has_data_perm", 0)),
                int(flags.get("licence_cleared", 0)),
                int(flags.get("load_only", 0)),
                int(flags.get("constant_output", 0)),
                int(flags.get("input_dependent", 0)),
                int(flags.get("different_student", 0)),
                as_c_digest(flags.get("model_sha256")),
                as_c_digest(flags.get("weights_sha256")),
                as_c_digest(flags.get("input_sha256")),
                expected_buf,
                len(expected),
            )
        )

    def bind(self, model: str, weights: str, inp: str) -> bytes:
        out = (c_uint8 * 32)()
        self.lib.k1_useful_npu_bind_identity(as_c_digest(model), as_c_digest(weights), as_c_digest(inp), out)
        return bytes(out)

    def snapshot(self) -> dict[str, Any]:
        identity = (c_uint8 * 32)()
        self.lib.k1_useful_npu_copy_identity(identity)
        g6 = self.lib.k1_useful_npu_g6_text()
        return {
            "admitted": bool(self.lib.k1_useful_npu_admitted()),
            "last_decline": int(self.lib.k1_useful_npu_last_decline()),
            "last_decline_name": DECLINE[int(self.lib.k1_useful_npu_last_decline())],
            "g6_no_qualifying": bool(self.lib.k1_useful_npu_g6_no_qualifying()),
            "g6": g6.decode("ascii") if isinstance(g6, bytes) else str(g6),
            "licence_unknown": bool(self.lib.k1_useful_npu_licence_unknown()),
            "invoked": bool(self.lib.k1_useful_npu_invoked()),
            "identity": bytes(identity).hex(),
            "identity_set": bool(self.lib.k1_useful_npu_identity_set()),
        }


def load_host_npu(directory: Path | None = None) -> tuple[HostUsefulNpu, dict[str, Any]]:
    if directory is None:
        directory = Path(tempfile.mkdtemp(prefix="k1-useful-npu-host-"))
    suffix = "dylib" if sys.platform == "darwin" else "so"
    library_path = directory / f"libuseful_npu_probe.{suffix}"
    compile_receipt = compile_host_library(library_path)
    return HostUsefulNpu(CDLL(str(library_path))), compile_receipt


def locate_original_artefacts(root: Path = FACE_DETECT) -> dict[str, Any]:
    present_generated = {}
    for rel in GENERATED_C:
        path = root / rel
        present_generated[rel] = {
            "exists": path.is_file(),
            "sha256": sha256_file(path) if path.is_file() else None,
            "bytes": path.stat().st_size if path.is_file() else 0,
        }
    found = {
        "tflite": sorted(str(p.relative_to(root)) for p in root.rglob("*.tflite") if p.is_file()) if root.is_dir() else [],
        "onnx": sorted(str(p.relative_to(root)) for p in root.rglob("*.onnx") if p.is_file()) if root.is_dir() else [],
        "npy": sorted(str(p.relative_to(root)) for p in root.rglob("*.npy") if p.is_file()) if root.is_dir() else [],
        "vela": sorted(
            str(p.relative_to(root))
            for p in root.rglob("*")
            if p.is_file() and "vela" in p.name.lower()
        )
        if root.is_dir()
        else [],
        "golden": sorted(
            str(p.relative_to(root))
            for p in root.rglob("*")
            if p.is_file() and "golden" in p.name.lower()
        )
        if root.is_dir()
        else [],
        "allocation": sorted(
            str(p.relative_to(root))
            for p in root.rglob("*")
            if p.is_file() and "alloc" in p.name.lower() and p.suffix in {".json", ".txt", ".csv", ".html"}
        )
        if root.is_dir()
        else [],
    }
    generated_extents = {
        "arena_bytes": 442368,
        "model_data_bytes": 440048,
        "input_bytes": 36864,
        "output0_bytes": 2592,
        "output1_bytes": 648,
        "source": "src/models/sub_0000_invoke.c generated comments",
        "is_original_allocation_report": False,
    }
    missing = {
        "same_compilation_tflite": not found["tflite"],
        "source_onnx": not found["onnx"],
        "allocation_report": not found["allocation"],
        "operator_fallback_report": True,
        "golden_inputs": not found["golden"] and not found["npy"],
        "golden_outputs": not found["golden"] and not found["npy"],
        "separate_code_weight_data_permissions": True,
        "original_model_licence": True,
    }
    return {
        "root": str(root),
        "exists": root.is_dir(),
        "bsp_commit": BSP_COMMIT,
        "student": "Titan_Mini_npu_ai_face_detection YOLO-Fastest 192x192 INT8",
        "generated_c": present_generated,
        "found_original_artefacts": found,
        "generated_c_extents_observation": generated_extents,
        "missing": missing,
        "licence": "UNKNOWN",
        "licence_cleared": False,
        "do_not_substitute": [
            {"role": "identified-platform-load-only", "tflite_sha256": LOAD_ONLY_TFLITE_SHA256, "onnx_sha256": LOAD_ONLY_ONNX_SHA256},
            {"role": "ShareStudent-semantic-checkpoint", "reason": "G6 already NO_QUALIFYING_CANDIDATE; not this acquisition lead"},
        ],
    }


def acquisition_request(inspection: Mapping[str, Any]) -> dict[str, Any]:
    return {
        "schema_version": 2,
        "receipt_id": "wp14-useful-u55-acquisition",
        "qualification_level": "HOST",
        "status": "BLOCKED",
        "g6": G6,
        "licence": "UNKNOWN",
        "candidate": inspection["student"],
        "bsp_commit": BSP_COMMIT,
        "request": [
            "Original same-compilation TensorFlow Lite INT8 that produced sub_0000_command_stream.c and sub_0000_model_data.c for Titan_Mini_npu_ai_face_detection (YOLO-Fastest, 192x192).",
            "Compiler/runtime identity for that compilation: Vela or RUHMI version, Ethos-U driver version, FSP version.",
            "Operator list and CPU-fallback report from that compilation.",
            "Original allocation report (arena/SRAM extents as emitted by the compiler, not reconstructed from generated C).",
            "Golden input tensors and golden output tensors bound to that compilation.",
            "Separate code / weight / data permission map for the compiled graph.",
            "Original model licence. Generated EdgeCortix/TF Apache-2.0 headers do not clear the student weights. UNKNOWN stays UNKNOWN.",
        ],
        "present_but_insufficient": inspection["generated_c"],
        "generated_c_extents_are_not_allocation_report": True,
        "found_original_artefacts": inspection["found_original_artefacts"],
        "do_not_pick_a_different_student": True,
        "load_only_cannot_satisfy": "useful-inference",
        "load_only_graph": {
            "same_compilation_tflite_sha256": LOAD_ONLY_TFLITE_SHA256,
            "source_onnx_sha256": LOAD_ONLY_ONNX_SHA256,
            "expected_raw_output": LOAD_ONLY_OUTPUT,
            "role": "identified-platform-load-only",
        },
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
    return missing


def make_receipt(
    *,
    qualification_level: str,
    npu: Mapping[str, Any],
    identities: Mapping[str, Any] | None = None,
    execution: Mapping[str, Any] | None = None,
    tools: list[dict[str, Any]] | None = None,
    limitations: list[str] | None = None,
    receipt_id: str = "wp14-useful-npu",
    claims_useful_inference: bool = False,
) -> dict[str, Any]:
    if qualification_level not in LEVELS:
        raise UsefulNpuError("qualification_level", f"Unknown {qualification_level}.")
    identities = dict(identities or {})
    execution = dict(execution or {})
    errors: list[dict[str, str]] = []
    blocked = ["u55-useful-inference", "u55-current-target"]
    load_only = execution.get("load_only") is True or str(execution.get("npu_role") or "").lower() in {
        "load-only",
        "identified-platform-load-only",
    }
    constant = list(npu.get("expected_raw_output") or []) == LOAD_ONLY_OUTPUT
    if load_only or constant:
        if claims_useful_inference or npu.get("useful_inference") is True:
            errors.append(
                {
                    "code": "load-only-as-useful-inference",
                    "message": "Existing load-only NPU graph cannot satisfy useful input-dependent inference",
                }
            )
    if npu.get("g6") not in (None, G6) and not npu.get("original_goldens_located"):
        errors.append(
            {
                "code": "g6-rewritten-without-goldens",
                "message": "G6 cannot leave NO_QUALIFYING_CANDIDATE without original same-compilation goldens",
            }
        )
    if npu.get("licence") not in (None, "UNKNOWN") and not npu.get("licence_cleared_evidence"):
        errors.append(
            {
                "code": "licence-unknown-cleared",
                "message": "Model licence UNKNOWN cannot become cleared without evidence",
            }
        )
    if npu.get("different_student") is True:
        errors.append(
            {
                "code": "different-student-selected",
                "message": "Do not pick a different student when original goldens are missing",
            }
        )
    if qualification_level == "CURRENT_TARGET":
        for field in missing_current_target_fields(identities):
            errors.append({"code": "current-target-identity-missing", "message": field})
        if execution.get("arm_binary_executed") is not True:
            errors.append({"code": "u55-not-executed", "message": "CURRENT_TARGET useful U55 requires identified-target execution"})
        ownership = identities.get("ownership_record")
        if isinstance(ownership, Mapping) and ownership.get("method") == "port-name":
            errors.append({"code": "port-name-is-not-identity", "message": "USB/serial port names are not board identity"})
        if execution.get("compiler_facts_only"):
            errors.append({"code": "compiler-facts-as-current-target", "message": "Build-only facts cannot claim CURRENT_TARGET U55"})
    status = "PASS" if not errors else "FAIL"
    if qualification_level == "CURRENT_TARGET" and errors:
        status = "FAIL_CLOSED"
    return {
        "schema_version": 2,
        "receipt_id": receipt_id,
        "recipe": {"id": "useful-npu", "version": "1"},
        "contract_version": "RA8P1-SOL-1.0.0",
        "qualification_level": qualification_level,
        "execution": execution,
        "identities": identities,
        "npu": dict(npu),
        "claims_useful_inference": claims_useful_inference,
        "p99_pass_fail": "unscored",
        "p99_us": 6000,
        "g6": npu.get("g6", G6),
        "tools": tools or [],
        "errors": errors,
        "blocked_cells": blocked,
        "u55_current_target": "NOT_RUN",
        "ok": not errors,
        "status": status,
        "limitations": limitations
        or [
            "Load-only graph cannot satisfy useful inference.",
            "Original same-compilation goldens were not located.",
            "G6 remains NO_QUALIFYING_CANDIDATE.",
            "Licence UNKNOWN stays UNKNOWN.",
        ],
        "started_at": datetime.now(timezone.utc).isoformat(),
        "completed_at": datetime.now(timezone.utc).isoformat(),
    }


def current_target_not_run(npu: Mapping[str, Any] | None = None) -> dict[str, Any]:
    payload = dict(npu or {})
    payload.setdefault("g6", G6)
    payload.setdefault("licence", "UNKNOWN")
    payload.setdefault("original_goldens_located", False)
    return make_receipt(
        qualification_level="HOST",
        receipt_id="wp14-u55-current-target-not-run",
        identities={
            "last_identified_uid": LAST_IDENTIFIED_UID,
            "last_identified_build_id": LAST_IDENTIFIED_BUILD,
            "last_identified_level": "HISTORICAL_TARGET",
            "live_target": "NOT_VERIFIED",
            "live_loaded_image": "unknown-no-reenumeration",
        },
        execution={"arm_binary_executed": False, "hardware_session": False, "load_only": True, "npu_role": "identified-platform-load-only"},
        npu=payload,
        claims_useful_inference=False,
        limitations=[
            "live_target is NOT_VERIFIED; last UID is HISTORICAL_TARGET.",
            "CURRENT_TARGET useful U55 is NOT_RUN, not a host-pass.",
            "G6 remains NO_QUALIFYING_CANDIDATE.",
            "No USB, flash, serial, audio, cadence runner, or song loop.",
        ],
    )


def cross_compile(output: Path) -> dict[str, Any]:
    if not ARM_GCC.is_file():
        return {
            "qualification_level": "CROSS_COMPILED",
            "status": "NOT_RUN",
            "missing_tool": str(ARM_GCC),
            "ok": False,
            "executed": False,
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
            "executed": False,
            "linked": False,
        }
    return {
        "qualification_level": "CROSS_COMPILED",
        "status": "PASS",
        "ok": True,
        "command": command,
        "exit_code": 0,
        "compiler": compiler,
        "object": str(output),
        "object_sha256": sha256_file(output),
        "source_sha256": sha256_file(SOURCE_C),
        "executed": False,
        "linked": False,
        "cannot_satisfy": "useful-inference",
    }


def write_json(path: Path, payload: Mapping[str, Any]) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    blob = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    path.write_text(blob, encoding="utf-8")
    return sha256_bytes(blob.encode("utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=PLUGIN_DIR)
    args = parser.parse_args()
    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    inspection = locate_original_artefacts()
    request = acquisition_request(inspection)
    write_json(output / "useful-u55-acquisition-request.json", request)
    npu_host, host_compile = load_host_npu()
    load_rc = npu_host.admit(
        load_only=1,
        constant_output=1,
        expected_raw=LOAD_ONLY_OUTPUT,
        model_sha256=LOAD_ONLY_TFLITE_SHA256,
    )
    face_rc = npu_host.admit(
        has_same_compilation_model=0,
        has_allocation_report=0,
        has_operator_report=0,
        has_golden_inputs=0,
        has_golden_outputs=0,
        licence_cleared=0,
        load_only=0,
        input_dependent=0,
    )
    probe_rc = int(npu_host.lib.k1_useful_npu_probe())
    invoke_rc = int(npu_host.lib.k1_useful_npu_invoke())
    snap = npu_host.snapshot()
    npu_payload = {
        "g6": G6,
        "licence": "UNKNOWN",
        "original_goldens_located": False,
        "load_only_decline": DECLINE[load_rc],
        "face_detector_decline": DECLINE[face_rc],
        "probe_decline": DECLINE[probe_rc],
        "invoke_rc": invoke_rc,
        "snapshot": snap,
        "inspection": inspection,
        "expected_raw_output": LOAD_ONLY_OUTPUT,
    }
    host_receipt = make_receipt(
        qualification_level="HOST",
        receipt_id="wp14-useful-npu-host",
        identities={"adapter_id": ADAPTER_ID, "bsp_commit": BSP_COMMIT},
        execution={"load_only": True, "npu_role": "identified-platform-load-only", "arm_binary_executed": False},
        npu=npu_payload,
        claims_useful_inference=False,
        tools=[{"name": "cc", "argv": host_compile["command"], "exit_code": 0, "sha256": host_compile["sha256"]}],
    )
    write_json(output / "useful-npu-host-receipt.json", host_receipt)
    with tempfile.TemporaryDirectory(prefix="k1-useful-npu-cross-") as temp:
        cross = cross_compile(Path(temp) / "useful_npu_probe.o")
        if cross.get("object"):
            dest = output / "useful_npu_probe.cross.o"
            dest.write_bytes(Path(cross["object"]).read_bytes())
            cross["object"] = str(dest)
            cross["object_sha256"] = sha256_file(dest)
        write_json(output / "useful-npu-cross-compiled-receipt.json", cross)
    physical = current_target_not_run(npu_payload)
    write_json(output / "useful-npu-current-target-not-run.json", physical)
    print(
        json.dumps(
            {
                "host": host_receipt["status"],
                "load_only": DECLINE[load_rc],
                "face": DECLINE[face_rc],
                "g6": snap["g6"],
                "licence": "UNKNOWN" if snap["licence_unknown"] else "CLEARED",
                "current_target": physical["u55_current_target"],
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
