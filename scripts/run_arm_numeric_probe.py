#!/usr/bin/env python3
"""HOST scalar/MVE fixtures, independent reference, fail-closed Arm receipts.

Compiles platform/ra8p1/arm_numeric_probe.c for the host. Does not open USB,
serial, flash, or audio. CROSS_COMPILED is compile-only plus objdump bind.
CURRENT_TARGET Arm execution stays NOT_RUN. Host/Python numerics cannot pass
as executed Arm. Frozen absolute tolerance is 1e-05; it is not widened.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import random
import re
import struct
import subprocess
import sys
import tempfile
from ctypes import CDLL, POINTER, c_float, c_int, c_uint32
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping

from verify_imports import ROOT

ADAPTER_ID = "k1-ra8p1-arm-numeric-v1"
LEVELS = ("HOST", "CROSS_COMPILED", "HISTORICAL_TARGET", "CURRENT_TARGET")
PREDECLARED_ABS_TOL = 1e-05
CAMPAIGN_SEED = 0x8A8F1
CAMPAIGN_COUNT = 256
SOURCE_C = ROOT / "platform/ra8p1/arm_numeric_probe.c"
PLUGIN_DIR = ROOT / "docs/evidence/K1-RA8P1-002/plugin-compute"
ARM_GCC = (
    ROOT.parent
    / "toolchains/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin/arm-none-eabi-gcc"
)
ARM_OBJDUMP = ARM_GCC.with_name("arm-none-eabi-objdump")
LAST_IDENTIFIED_UID = "545433931bd25436593630352d068363"
LAST_IDENTIFIED_BUILD = "c4ceebe7f4d899d39a917fb12c385c0f743278fd53aa2a82045230f3490e87bb"
PHYSICAL_BLOCKERS = (
    "re-enumeration of UID",
    "exact loaded-image identity",
    "exclusive ownership record",
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
SCALAR_CFLAGS = (
    "-mcpu=cortex-m85+nomve",
    "-mthumb",
    "-mfpu=fpv5-sp-d16",
    "-mfloat-abi=hard",
    "-O2",
    "-ffp-contract=off",
    "-fno-fast-math",
    "-fno-tree-vectorize",
    "-fno-tree-slp-vectorize",
)
MVE_CFLAGS = (
    "-mcpu=cortex-m85",
    "-mthumb",
    "-mfloat-abi=hard",
    "-O2",
    "-ffp-contract=off",
    "-fno-fast-math",
)
CLASS_NAMES = {
    0: "finite",
    1: "nan",
    2: "+inf",
    3: "-inf",
    4: "-0",
    5: "+0",
    6: "denormal",
}
SPECIAL_SEMANTICS = {
    0: {"name": "nan_times_one", "expect_class": 1, "score_with_abs_tol": False},
    1: {"name": "pos_inf_times_two", "expect_class": 2, "score_with_abs_tol": False},
    2: {"name": "neg_inf_times_two", "expect_class": 3, "score_with_abs_tol": False},
    3: {"name": "pos_inf_times_zero", "expect_class": 1, "score_with_abs_tol": False},
    4: {"name": "pos_zero_times_one", "expect_class": 5, "score_with_abs_tol": False},
    5: {"name": "neg_zero_times_one", "expect_class": (4, 5), "score_with_abs_tol": False},
    6: {"name": "denormal_times_two", "expect_class": (0, 6), "score_with_abs_tol": False},
}


class ArmNumericError(ValueError):
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


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def independent_dot_f64(left: list[float], right: list[float]) -> float:
    return math.fsum(float(a) * float(b) for a, b in zip(left, right))


def campaign_vectors() -> tuple[list[float], list[float]]:
    generator = random.Random(CAMPAIGN_SEED)
    left = [f32(generator.uniform(-1.0, 1.0)) for _ in range(CAMPAIGN_COUNT)]
    right = [f32(generator.uniform(-1.0, 1.0)) for _ in range(CAMPAIGN_COUNT)]
    return left, right


def python_f32_accum(left: list[float], right: list[float]) -> float:
    total = f32(0.0)
    for a, b in zip(left, right):
        total = f32(total + f32(a * b))
    return total


def as_c_floats(values: list[float]):
    buf = (c_float * len(values))()
    for i, value in enumerate(values):
        buf[i] = c_float(value)
    return buf


def compile_host_library(output: Path) -> dict[str, Any]:
    command = [
        "cc",
        "-std=c11",
        "-shared",
        "-fPIC",
        "-O2",
        "-ffp-contract=off",
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
        raise ArmNumericError(
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
        "arm_binary_executed": False,
    }


class HostArmNumeric:
    def __init__(self, library: CDLL) -> None:
        self.lib = library
        library.k1_arm_dot_scalar.argtypes = [POINTER(c_float), POINTER(c_float), c_uint32]
        library.k1_arm_dot_scalar.restype = c_float
        library.k1_arm_dot_chunked.argtypes = [POINTER(c_float), POINTER(c_float), c_uint32]
        library.k1_arm_dot_chunked.restype = c_float
        library.k1_arm_classify.argtypes = [c_float]
        library.k1_arm_classify.restype = c_int
        library.k1_arm_fill_cancellation.argtypes = [POINTER(c_float), POINTER(c_float), c_uint32]
        library.k1_arm_fill_cancellation.restype = c_int
        library.k1_arm_fill_magnitude.argtypes = [POINTER(c_float), POINTER(c_float), c_uint32]
        library.k1_arm_fill_magnitude.restype = c_int
        library.k1_arm_fill_tail.argtypes = [POINTER(c_float), POINTER(c_float), POINTER(c_uint32)]
        library.k1_arm_fill_tail.restype = c_int
        library.k1_arm_fill_special.argtypes = [c_uint32, POINTER(c_float), POINTER(c_float), POINTER(c_uint32)]
        library.k1_arm_fill_special.restype = c_int
        library.k1_arm_special_case_count.argtypes = []
        library.k1_arm_special_case_count.restype = c_int
        library.k1_arm_numeric_prepare.argtypes = []
        library.k1_arm_numeric_prepare.restype = c_int
        library.k1_arm_binary_executed.argtypes = []
        library.k1_arm_binary_executed.restype = c_int
        library.k1_arm_f32_bits.argtypes = [c_float]
        library.k1_arm_f32_bits.restype = c_uint32

    def dot_scalar(self, left: list[float], right: list[float]) -> float:
        a = as_c_floats(left)
        b = as_c_floats(right)
        return float(self.lib.k1_arm_dot_scalar(a, b, len(left)))

    def dot_chunked(self, left: list[float], right: list[float]) -> float:
        a = as_c_floats(left)
        b = as_c_floats(right)
        return float(self.lib.k1_arm_dot_chunked(a, b, len(left)))

    def fill_named(self, name: str, count: int = 12) -> tuple[list[float], list[float]]:
        a = (c_float * 256)()
        b = (c_float * 256)()
        n = c_uint32(count)
        if name == "cancellation":
            rc = self.lib.k1_arm_fill_cancellation(a, b, count)
        elif name == "magnitude":
            rc = self.lib.k1_arm_fill_magnitude(a, b, count)
        elif name == "tail":
            rc = self.lib.k1_arm_fill_tail(a, b, n)
            count = int(n.value)
        else:
            raise ArmNumericError("unknown_fixture", name)
        if rc != 0:
            raise ArmNumericError("fill_failed", f"{name} returned {rc}")
        return [a[i] for i in range(count)], [b[i] for i in range(count)]

    def special(self, case_id: int) -> tuple[list[float], list[float]]:
        a = (c_float * 4)()
        b = (c_float * 4)()
        n = c_uint32(0)
        rc = self.lib.k1_arm_fill_special(case_id, a, b, n)
        if rc != 0:
            raise ArmNumericError("fill_failed", f"special {case_id} returned {rc}")
        count = int(n.value)
        return [a[i] for i in range(count)], [b[i] for i in range(count)]


def load_host_arm(directory: Path | None = None) -> tuple[HostArmNumeric, dict[str, Any]]:
    close = False
    if directory is None:
        directory = Path(tempfile.mkdtemp(prefix="k1-arm-host-"))
        close = True
    suffix = "dylib" if sys.platform == "darwin" else "so"
    library_path = directory / f"libarm_numeric_probe.{suffix}"
    compile_receipt = compile_host_library(library_path)
    library = CDLL(str(library_path))
    if close:
        compile_receipt["temp_dir"] = str(directory)
    return HostArmNumeric(library), compile_receipt


def score_finite(error: float, claimed_tol: float | None = None) -> dict[str, Any]:
    claimed = PREDECLARED_ABS_TOL if claimed_tol is None else claimed_tol
    widened = claimed > PREDECLARED_ABS_TOL
    return {
        "absolute_error": error,
        "predeclared_abs_tol": PREDECLARED_ABS_TOL,
        "claimed_tol": claimed,
        "widened": widened,
        "within_predeclared": (not widened) and error <= PREDECLARED_ABS_TOL,
        "scored_as_executed_arm": False,
    }


def refuse_widened_tolerance(claimed_tol: float) -> dict[str, Any]:
    return {
        "ok": False,
        "code": "tolerance-widened-after-mismatch",
        "claimed_tol": claimed_tol,
        "predeclared_abs_tol": PREDECLARED_ABS_TOL,
    }


def fixture_record(name: str, left: list[float], right: list[float], host: HostArmNumeric) -> dict[str, Any]:
    scalar = host.dot_scalar(left, right)
    chunked = host.dot_chunked(left, right)
    reference = independent_dot_f64(left, right)
    python_acc = python_f32_accum(left, right)
    payload = {
        "name": name,
        "count": len(left),
        "input_sha256": sha256_bytes(
            b"".join(struct.pack("<f", f32(v)) for v in left + right)
        ),
        "host_scalar": scalar,
        "host_chunked": chunked,
        "independent_f64": reference,
        "python_f32_accum": python_acc,
        "scalar_vs_ref": abs(scalar - reference),
        "chunked_vs_ref": abs(chunked - reference),
        "python_vs_ref": abs(python_acc - reference),
        "qualification_level": "HOST",
        "arm_binary_executed": False,
        "host_classify_scalar": CLASS_NAMES[int(host.lib.k1_arm_classify(c_float(scalar)))],
    }
    if name == "campaign":
        # Frozen 1e-05 is the existing scalar-versus-chunked HOST bound.
        # Independent f64 is recorded and is not a new scored Arm contract.
        payload["score"] = score_finite(abs(scalar - chunked))
        payload["independent_ref_recorded"] = True
        payload["independent_ref_not_executed_arm"] = True
    else:
        payload["score"] = {
            "scored": False,
            "reason": "Prepared fixture; new numerical contract would need Astra before Arm scoring.",
            "predeclared_abs_tol": PREDECLARED_ABS_TOL,
            "not_widened": True,
        }
    return payload


def run_host_fixtures(host: HostArmNumeric) -> dict[str, Any]:
    left, right = campaign_vectors()
    campaign = fixture_record("campaign", left, right, host)
    cancel_l, cancel_r = host.fill_named("cancellation", 12)
    mag_l, mag_r = host.fill_named("magnitude", 16)
    tail_l, tail_r = host.fill_named("tail")
    specials = []
    for case_id, spec in SPECIAL_SEMANTICS.items():
        s_left, s_right = host.special(case_id)
        observed = host.dot_scalar(s_left, s_right)
        observed_class = int(host.lib.k1_arm_classify(c_float(observed)))
        expect = spec["expect_class"]
        expect_ok = observed_class in expect if isinstance(expect, tuple) else observed_class == expect
        specials.append(
            {
                "case_id": case_id,
                "name": spec["name"],
                "expect_class": expect,
                "observed_class": observed_class,
                "observed_class_name": CLASS_NAMES[observed_class],
                "class_match": expect_ok,
                "score_with_abs_tol": False,
                "arm_binary_executed": False,
            }
        )
    if int(host.lib.k1_arm_binary_executed()) != 0:
        raise ArmNumericError("host_claimed_arm", "Host library set k1_arm_binary_executed.")
    return {
        "campaign": campaign,
        "cancellation": fixture_record("cancellation", cancel_l, cancel_r, host),
        "magnitude": fixture_record("magnitude", mag_l, mag_r, host),
        "tail": fixture_record("tail", tail_l, tail_r, host),
        "specials": specials,
        "semantics": {
            "finite": "Campaign seed 0x8A8F1 / 256 samples uses frozen 1e-05 absolute tolerance on HOST only.",
            "cancellation": "Alternating 1e8, 1, -1e8 against ones; HOST observation, not Arm pass.",
            "magnitude": "1e8*1e-20 mixed with 1e-20*1e8; HOST observation, not Arm pass.",
            "tails": "Count 5, not a multiple of 4; leftover scalar loop is required.",
            "specials": SPECIAL_SEMANTICS,
            "independent_reference": "math.fsum of Python-float (IEEE binary64) products, not C accumulation.",
        },
        "qualification_level": "HOST",
        "arm_binary_executed": False,
    }


def extract_function(disassembly: str, name: str) -> str | None:
    pattern = re.compile(r"^[0-9a-fA-F]+ <([^>]+)>:")
    lines = disassembly.splitlines()
    start = None
    for i, line in enumerate(lines):
        match = pattern.match(line.strip())
        if match and match.group(1) == name:
            start = i
            break
    if start is None:
        return None
    body = [lines[start]]
    for line in lines[start + 1 :]:
        if pattern.match(line.strip()):
            break
        body.append(line)
    return "\n".join(body)


def bind_disassembly(disassembly: str, required: str, require_mve: bool) -> dict[str, Any]:
    body = extract_function(disassembly, required)
    if body is None:
        return {
            "ok": False,
            "code": "wrong-function-disassembly",
            "required": required,
            "present": False,
        }
    lowered = body.lower()
    has_mve = "vfma.f32\tq" in lowered or "vfma.f32\t q" in lowered
    if require_mve and not has_mve:
        return {
            "ok": False,
            "code": "mve-marker-missing",
            "required": required,
            "body_sha256": sha256_bytes(body.encode("utf-8")),
            "has_mve_vfma_q": False,
        }
    if (not require_mve) and has_mve:
        return {
            "ok": False,
            "code": "scalar-function-contains-mve",
            "required": required,
            "has_mve_vfma_q": True,
        }
    return {
        "ok": True,
        "required": required,
        "present": True,
        "has_mve_vfma_q": has_mve,
        "body_sha256": sha256_bytes(body.encode("utf-8")),
        "executed": False,
    }


def cross_compile(kind: str, output: Path, disasm: Path) -> dict[str, Any]:
    if not ARM_GCC.is_file() or not ARM_OBJDUMP.is_file():
        return {
            "qualification_level": "CROSS_COMPILED",
            "status": "NOT_RUN",
            "kind": kind,
            "missing_tool": str(ARM_GCC if not ARM_GCC.is_file() else ARM_OBJDUMP),
            "ok": False,
            "arm_binary_executed": False,
        }
    flags = SCALAR_CFLAGS if kind == "scalar" else MVE_CFLAGS
    command = [
        str(ARM_GCC),
        "-std=c11",
        "-c",
        *flags,
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
            "kind": kind,
            "ok": False,
            "command": command,
            "exit_code": completed.returncode,
            "stderr": completed.stderr,
            "compiler": compiler,
            "arm_binary_executed": False,
            "executed": False,
            "linked": False,
        }
    dump = subprocess.run(
        [str(ARM_OBJDUMP), "-d", str(output)],
        check=False,
        capture_output=True,
        text=True,
    )
    if dump.returncode != 0:
        return {
            "qualification_level": "CROSS_COMPILED",
            "status": "FAIL",
            "kind": kind,
            "ok": False,
            "objdump_stderr": dump.stderr,
            "arm_binary_executed": False,
        }
    disasm.write_text(dump.stdout, encoding="utf-8")
    required = "k1_arm_dot_scalar" if kind == "scalar" else "k1_arm_dot_mve"
    bind = bind_disassembly(dump.stdout, required, require_mve=(kind == "mve"))
    other = extract_function(dump.stdout, "k1_arm_dot_mve" if kind == "scalar" else "k1_arm_dot_scalar")
    return {
        "qualification_level": "CROSS_COMPILED",
        "status": "PASS" if bind["ok"] else "FAIL",
        "ok": bind["ok"],
        "kind": kind,
        "command": command,
        "exit_code": 0,
        "compiler": compiler,
        "compiler_path": str(ARM_GCC),
        "object": str(output),
        "object_sha256": sha256_file(output),
        "disassembly": str(disasm),
        "disassembly_sha256": sha256_file(disasm),
        "source_sha256": sha256_file(SOURCE_C),
        "bind": bind,
        "other_function_present": other is not None,
        "executed": False,
        "linked": False,
        "arm_binary_executed": False,
        "limitations": [
            "Compile-only. The object was not linked or executed.",
            "CROSS_COMPILED disassembly is not CURRENT_TARGET Arm numerics.",
        ],
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
    numerical: Mapping[str, Any],
    identities: Mapping[str, Any] | None = None,
    execution: Mapping[str, Any] | None = None,
    tools: list[dict[str, Any]] | None = None,
    limitations: list[str] | None = None,
    receipt_id: str = "wp14-arm-numeric",
    claims_executed_arm: bool = False,
    claimed_tol: float | None = None,
) -> dict[str, Any]:
    if qualification_level not in LEVELS:
        raise ArmNumericError("qualification_level", f"Unknown qualification_level {qualification_level}.")
    identities = dict(identities or {})
    execution = dict(execution or {})
    errors: list[dict[str, str]] = []
    blocked: list[str] = ["arm-current-target"]
    python_host = bool(execution.get("python_numerics") or str(execution.get("host_language") or "").lower() in {"python", "cpython"})
    arm_executed = execution.get("arm_binary_executed") is True
    if python_host:
        blocked.append("arm-executed-numerics")
    if execution.get("arm_binary_executed") is False:
        blocked.append("arm-executed-numerics")
    if python_host and (claims_executed_arm or arm_executed):
        errors.append(
            {
                "code": "python-numerics-as-executed-arm",
                "message": "Python host numerics cannot satisfy executed Arm scalar/MVE",
            }
        )
    elif claims_executed_arm and not arm_executed:
        errors.append(
            {
                "code": "python-numerics-as-executed-arm",
                "message": "Host accumulation cannot pass as executed Arm",
            }
        )
    if claimed_tol is not None and claimed_tol > PREDECLARED_ABS_TOL:
        errors.append(
            {
                "code": "tolerance-widened-after-mismatch",
                "message": f"claimed_tol {claimed_tol} exceeds frozen {PREDECLARED_ABS_TOL}",
            }
        )
    if (execution.get("compiler_facts_only") or qualification_level == "CROSS_COMPILED") and not arm_executed:
        blocked.append("arm-executed-numerics")
        if claims_executed_arm or qualification_level == "CURRENT_TARGET":
            errors.append(
                {
                    "code": "disassembly-as-executed-arm",
                    "message": "Compiled disassembly alone cannot pass Arm numerics",
                }
            )
    if qualification_level == "CURRENT_TARGET":
        for field in missing_current_target_fields(identities):
            errors.append({"code": "current-target-identity-missing", "message": field})
        if not arm_executed:
            errors.append(
                {
                    "code": "arm-binary-not-executed",
                    "message": "CURRENT_TARGET Arm numerics require arm_binary_executed",
                }
            )
        ownership = identities.get("ownership_record")
        if isinstance(ownership, Mapping) and ownership.get("method") == "port-name":
            errors.append({"code": "port-name-is-not-identity", "message": "USB/serial port names are not board identity"})
        if execution.get("compiler_facts_only"):
            errors.append(
                {
                    "code": "compiler-facts-as-current-target",
                    "message": "Build-only/compiler facts cannot claim CURRENT_TARGET completion",
                }
            )
    status = "PASS" if not errors else "FAIL"
    if qualification_level == "CURRENT_TARGET" and errors:
        status = "FAIL_CLOSED"
    return {
        "schema_version": 2,
        "receipt_id": receipt_id,
        "recipe": {"id": "arm-numeric", "version": "1"},
        "contract_version": "RA8P1-SOL-1.0.0",
        "qualification_level": qualification_level,
        "execution": execution,
        "identities": identities,
        "numerical": dict(numerical),
        "claims_executed_arm": claims_executed_arm,
        "p99_pass_fail": "unscored",
        "p99_us": 6000,
        "tools": tools or [],
        "errors": errors,
        "blocked_cells": sorted(set(blocked)),
        "arm_current_target": "NOT_RUN",
        "ok": not errors,
        "status": status,
        "limitations": limitations
        or [
            "HOST/CROSS_COMPILED work does not execute Arm on the identified target.",
            "Independent reference is binary64 math.fsum, not C accumulation.",
            "Qualification remains engineering-preview.",
        ],
        "started_at": datetime.now(timezone.utc).isoformat(),
        "completed_at": datetime.now(timezone.utc).isoformat(),
    }


def current_target_not_run(numerical: Mapping[str, Any] | None = None) -> dict[str, Any]:
    return make_receipt(
        qualification_level="HOST",
        receipt_id="wp14-arm-current-target-not-run",
        identities={
            "last_identified_uid": LAST_IDENTIFIED_UID,
            "last_identified_build_id": LAST_IDENTIFIED_BUILD,
            "last_identified_level": "HISTORICAL_TARGET",
            "live_target": "NOT_VERIFIED",
            "live_loaded_image": "unknown-no-reenumeration",
        },
        execution={"arm_binary_executed": False, "hardware_session": False, "python_numerics": False},
        numerical=numerical or {"fixtures_prepared": True, "arm_binary_executed": False},
        limitations=[
            "live_target is NOT_VERIFIED; last UID is HISTORICAL_TARGET.",
            "CURRENT_TARGET Arm scalar/MVE cells are NOT_RUN, not a host-pass.",
            "Blocked until: " + "; ".join(PHYSICAL_BLOCKERS) + ".",
            "No USB, flash, serial, audio, cadence runner, or song loop.",
        ],
    )


def write_json(path: Path, payload: Mapping[str, Any]) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    blob = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    path.write_text(blob, encoding="utf-8")
    return sha256_bytes(blob.encode("utf-8"))


def build_adapter_document() -> dict[str, Any]:
    return {
        "id": ADAPTER_ID,
        "version": "1",
        "adapter_class": "arm_numeric_probe",
        "qualification_level": "HOST",
        "source": "platform/ra8p1/arm_numeric_probe.c",
        "source_sha256": sha256_file(SOURCE_C),
        "independent_reference": "math.fsum binary64 products",
        "predeclared_abs_tol": PREDECLARED_ABS_TOL,
        "campaign_seed": hex(CAMPAIGN_SEED),
        "campaign_count": CAMPAIGN_COUNT,
        "cannot_satisfy": "executed-arm",
        "note": "Prepared scalar/chunked/MVE fixtures. Host C is HOST. Disassembly is CROSS_COMPILED.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=PLUGIN_DIR)
    args = parser.parse_args()
    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    adapter = build_adapter_document()
    write_json(output / "adapter-k1-ra8p1-arm-numeric-v1.json", adapter)
    host, host_compile = load_host_arm()
    fixtures = run_host_fixtures(host)
    host_receipt = make_receipt(
        qualification_level="HOST",
        receipt_id="wp14-arm-host-fixtures",
        identities={"adapter_id": ADAPTER_ID, "source_sha256": sha256_file(SOURCE_C)},
        execution={
            "arm_binary_executed": False,
            "compiler_facts_only": False,
            "host_library": True,
            "python_numerics": False,
            "host_language": "c",
        },
        numerical=fixtures,
        tools=[{"name": "cc", "argv": host_compile["command"], "exit_code": 0, "sha256": host_compile["sha256"]}],
        claims_executed_arm=False,
    )
    write_json(output / "arm-host-receipt.json", host_receipt)
    with tempfile.TemporaryDirectory(prefix="k1-arm-cross-") as temp:
        scalar = cross_compile("scalar", Path(temp) / "arm_numeric_probe.scalar.o", Path(temp) / "scalar.disasm")
        mve = cross_compile("mve", Path(temp) / "arm_numeric_probe.mve.o", Path(temp) / "mve.disasm")
        for item, name in ((scalar, "arm_numeric_probe.scalar.cross.o"), (mve, "arm_numeric_probe.mve.cross.o")):
            if item.get("object"):
                dest = output / name
                dest.write_bytes(Path(item["object"]).read_bytes())
                item["object"] = str(dest)
                item["object_sha256"] = sha256_file(dest)
            if item.get("disassembly"):
                dest = output / (name.replace(".cross.o", ".disasm.txt"))
                dest.write_text(Path(item["disassembly"]).read_text(encoding="utf-8"), encoding="utf-8")
                item["disassembly"] = str(dest)
                item["disassembly_sha256"] = sha256_file(dest)
        cross = {
            "qualification_level": "CROSS_COMPILED",
            "ok": bool(scalar.get("ok") and mve.get("ok")),
            "status": "PASS" if scalar.get("ok") and mve.get("ok") else "FAIL",
            "scalar": scalar,
            "mve": mve,
            "executed": False,
            "linked": False,
            "arm_binary_executed": False,
            "cannot_satisfy": "executed-arm",
        }
        write_json(output / "arm-cross-compiled-receipt.json", cross)
    physical = current_target_not_run(fixtures)
    write_json(output / "arm-current-target-not-run.json", physical)
    print(
        json.dumps(
            {
                "host": host_receipt["status"],
                "cross": cross.get("status"),
                "current_target": physical["arm_current_target"],
                "campaign_within_tol": fixtures["campaign"]["score"]["within_predeclared"],
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
