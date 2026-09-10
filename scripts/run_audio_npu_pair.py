#!/usr/bin/env python3
"""Paired audio/NPU coexistence: mismatch reject, load-only refused, physical NOT_RUN.

HOST bookkeeping only. Does not open USB, serial, flash, or audio.
CROSS_COMPILED is compile-only. CURRENT_TARGET is refused without live exclusive
identity, physical PDM, and useful-U55 admission. 7500 us is not widened.
6000 us p99 stays unscored. G4 O2/O3 remain FAILED, not unrun.
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
from typing import Any, Mapping, Sequence

from verify_imports import ROOT

ADAPTER_ID = "k1-ra8p1-audio-npu-pair-v1"
LEVELS = ("HOST", "CROSS_COMPILED", "HISTORICAL_TARGET", "CURRENT_TARGET")
SOURCE_C = ROOT / "platform/ra8p1/coexist_probe.c"
PLUGIN_DIR = ROOT / "docs/evidence/K1-RA8P1-002/plugin-coexist"
ADAPTER_PATH = PLUGIN_DIR / "adapter-k1-ra8p1-audio-npu-pair-v1.json"
PDM_NOT_RUN = ROOT / "docs/evidence/K1-RA8P1-002/plugin-pdm/physical-capture-not-run.json"
U55_NOT_RUN = ROOT / "docs/evidence/K1-RA8P1-002/plugin-compute/useful-npu-current-target-not-run.json"
NPU_PROFILE = ROOT / "docs/evidence/K1-RA8P1-002/g4-npu-coexist-profile.json"
SCALAR_PROFILE = ROOT / "docs/evidence/K1-RA8P1-002/g4-scalar-workload-profile.json"
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
PCM_SHA256 = "5970e8d26787f5c9ccdda50b95077d3da503b3826495ed7232d2854fecb8d394"
LOAD_ONLY_TFLITE_SHA256 = "1fbdbb2878f5223697f70366036bf6260026f2e7ebbdb0dd0e0987d726f96fed"
LOAD_ONLY_ONNX_SHA256 = "5d0097e83fe269acb6ce92c5ab84cfe897561d3858328f2f0c610262c3801bd6"
LOAD_ONLY_OUTPUT = [127, 117, 123]
DEADLINE_US = 7500
P99_US = 6000
SAMPLE_RATE_HZ = 24000
HOP_PERIOD_US = 7500
G4_O2_MISSES = 2005
G4_O3_MISSES = 2006
G4 = "FAILED"
G6 = "NO_QUALIFYING_CANDIDATE"
TREATMENT_AUDIO_ONLY = 0
TREATMENT_USEFUL_NPU = 1
DECLINE = {
    0: "OK",
    1: "MISMATCH_INPUT",
    2: "MISMATCH_PROFILE",
    3: "MISMATCH_BUILD",
    4: "MISMATCH_CLOCK",
    5: "TREATMENT_NOT_DECLARED",
    6: "LOAD_ONLY",
    7: "USEFUL_U55_NOT_ADMITTED",
    8: "PHYSICAL_PDM_NOT_RUN",
    9: "DEADLINE_WIDENED",
    10: "G4_RERUN_AS_UNRUN",
    11: "P99_INVENTED",
    12: "MISSING_RAW_EVENTS",
    13: "BAD_EVENT",
}
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
NAMED_PHYSICAL_FACTS = (
    "live_uid",
    "exclusive_owner",
    "useful_u55_admission",
    "physical_pdm",
)
PHYSICAL_BLOCKERS = (
    "re-enumeration of UID",
    "exclusive ownership record",
    "WP14 useful-U55 CURRENT_TARGET admission",
    "WP13 physical PDM",
)


class PairError(ValueError):
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


def observed_source_hashes() -> dict[str, str]:
    return {
        "application": sha256_file(SOURCE_C),
        "npu_coexist_profile": sha256_file(NPU_PROFILE) if NPU_PROFILE.is_file() else "",
        "scalar_profile": sha256_file(SCALAR_PROFILE) if SCALAR_PROFILE.is_file() else "",
        "pcm": PCM_SHA256,
    }


def load_adapter(path: Path = ADAPTER_PATH) -> dict[str, Any]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("id") != ADAPTER_ID:
        raise PairError("adapter_id", f"Adapter id {payload.get('id')!r} is not {ADAPTER_ID}.")
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


def build_adapter_document() -> dict[str, Any]:
    hashes = observed_source_hashes()
    return {
        "id": ADAPTER_ID,
        "version": "1",
        "adapter_class": "audio_npu_pair",
        "qualification_level": "HOST",
        "source": "platform/ra8p1/coexist_probe.c",
        "cannot_satisfy": "physical-audio-npu-pair",
        "authorised_deadline_us": DEADLINE_US,
        "p99_us": P99_US,
        "p99_status": "unscored",
        "g4_status": G4,
        "g4_o2_deadline_misses": G4_O2_MISSES,
        "g4_o3_deadline_misses": G4_O3_MISSES,
        "g6": G6,
        "load_only_cannot_satisfy": "useful-npu-treatment",
        "inputs": {
            "application": {"path": "platform/ra8p1/coexist_probe.c", "sha256": hashes["application"]},
            "npu_coexist_profile": {
                "path": "docs/evidence/K1-RA8P1-002/g4-npu-coexist-profile.json",
                "sha256": hashes["npu_coexist_profile"],
            },
            "pcm": {"sha256": hashes["pcm"]},
        },
        "note": "HOST pair gate. Physical pair is NOT_RUN. Not CURRENT_TARGET.",
    }


def independent_percentile(durations: Sequence[int], p: int) -> int:
    if not durations:
        raise PairError("missing-raw-events", "No raw durations")
    ordered = sorted(int(value) for value in durations)
    n = len(ordered)
    idx = (p * n + 99) // 100
    idx = max(1, min(n, idx))
    return ordered[idx - 1]


def independent_max(durations: Sequence[int]) -> int:
    if not durations:
        raise PairError("missing-raw-events", "No raw durations")
    return max(int(value) for value in durations)


def independent_misses(durations: Sequence[int], deadline_us: int = DEADLINE_US) -> int:
    return sum(1 for value in durations if int(value) > deadline_us)


def refuse_widened_deadline(claimed_deadline_us: int) -> dict[str, Any]:
    ok = claimed_deadline_us == DEADLINE_US
    return {
        "ok": ok,
        "code": None if ok else "deadline-widened",
        "claimed_deadline_us": claimed_deadline_us,
        "authorised_deadline_us": DEADLINE_US,
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
        raise PairError(
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


class HostPair:
    def __init__(self, library: CDLL) -> None:
        self.lib = library
        library.k1_coexist_reset.argtypes = []
        library.k1_coexist_bind.argtypes = [
            POINTER(c_uint8), POINTER(c_uint8), POINTER(c_uint8),
            c_uint32, c_uint32, c_uint32, c_uint32,
        ]
        library.k1_coexist_bind.restype = c_int
        library.k1_coexist_set_treatment.argtypes = [c_int]
        library.k1_coexist_set_treatment.restype = c_int
        library.k1_coexist_pair_comparable.argtypes = [
            POINTER(c_uint8), POINTER(c_uint8), POINTER(c_uint8), c_uint32, c_int,
            POINTER(c_uint8), POINTER(c_uint8), POINTER(c_uint8), c_uint32, c_int,
        ]
        library.k1_coexist_pair_comparable.restype = c_int
        library.k1_coexist_record_event.argtypes = [c_uint32, c_uint32, c_uint32]
        library.k1_coexist_record_event.restype = c_int
        library.k1_coexist_event_count.restype = c_uint32
        library.k1_coexist_duration_at.argtypes = [c_uint32]
        library.k1_coexist_duration_at.restype = c_uint32
        library.k1_coexist_release_at.argtypes = [c_uint32]
        library.k1_coexist_release_at.restype = c_uint32
        library.k1_coexist_complete_at.argtypes = [c_uint32]
        library.k1_coexist_complete_at.restype = c_uint32
        library.k1_coexist_overhead_at.argtypes = [c_uint32]
        library.k1_coexist_overhead_at.restype = c_uint32
        library.k1_coexist_missed_at.argtypes = [c_uint32]
        library.k1_coexist_missed_at.restype = c_uint32
        library.k1_coexist_deadline_misses.restype = c_uint32
        library.k1_coexist_percentile.argtypes = [c_uint32]
        library.k1_coexist_percentile.restype = c_uint32
        library.k1_coexist_max_us.restype = c_uint32
        library.k1_coexist_has_distribution.restype = c_int
        library.k1_coexist_p99_scored.restype = c_int
        library.k1_coexist_p99_allocation_us.restype = c_uint32
        library.k1_coexist_deadline_us.restype = c_uint32
        library.k1_coexist_hop_period_us.restype = c_uint32
        library.k1_coexist_sample_rate_hz.restype = c_uint32
        library.k1_coexist_clock_hz.restype = c_uint32
        library.k1_coexist_window_start_us.restype = c_uint32
        library.k1_coexist_window_end_us.restype = c_uint32
        library.k1_coexist_overhead_sum_us.restype = c_uint32
        library.k1_coexist_identity_set.restype = c_int
        library.k1_coexist_treatment.restype = c_int
        library.k1_coexist_last_decline.restype = c_int
        library.k1_coexist_admitted.restype = c_int
        library.k1_coexist_blockers.restype = c_uint32
        library.k1_coexist_physical_not_run.restype = c_int
        library.k1_coexist_g4_o2_misses.restype = c_uint32
        library.k1_coexist_g4_o3_misses.restype = c_uint32
        library.k1_coexist_g4_already_failed.restype = c_int
        library.k1_coexist_g4_status_text.restype = c_char_p
        library.k1_coexist_g6_text.restype = c_char_p
        library.k1_coexist_refuse_widen.argtypes = [c_uint32]
        library.k1_coexist_refuse_widen.restype = c_int
        library.k1_coexist_is_load_only_output.argtypes = [POINTER(c_uint8), c_uint32]
        library.k1_coexist_is_load_only_output.restype = c_int
        library.k1_coexist_copy_input.argtypes = [POINTER(c_uint8)]
        library.k1_coexist_copy_profile.argtypes = [POINTER(c_uint8)]
        library.k1_coexist_copy_build.argtypes = [POINTER(c_uint8)]
        library.k1_coexist_admit.argtypes = [c_int, c_int, c_int, c_int, c_int, c_uint32]
        library.k1_coexist_admit.restype = c_int
        library.k1_coexist_probe.argtypes = []
        library.k1_coexist_probe.restype = c_int
        library.k1_coexist_acquire.argtypes = []
        library.k1_coexist_acquire.restype = c_int

    def reset(self) -> None:
        self.lib.k1_coexist_reset()

    def bind(
        self,
        input_sha256: str,
        profile_sha256: str,
        build_sha256: str,
        sample_rate_hz: int = SAMPLE_RATE_HZ,
        clock_hz: int = 0,
        hop_period_us: int = HOP_PERIOD_US,
        deadline_us: int = DEADLINE_US,
    ) -> int:
        return int(
            self.lib.k1_coexist_bind(
                as_c_digest(input_sha256),
                as_c_digest(profile_sha256),
                as_c_digest(build_sha256),
                sample_rate_hz,
                clock_hz,
                hop_period_us,
                deadline_us,
            )
        )

    def set_treatment(self, treatment: int) -> int:
        return int(self.lib.k1_coexist_set_treatment(treatment))

    def pair_comparable(
        self,
        a: Mapping[str, Any],
        b: Mapping[str, Any],
    ) -> int:
        return int(
            self.lib.k1_coexist_pair_comparable(
                as_c_digest(a["input"]),
                as_c_digest(a["profile"]),
                as_c_digest(a["build"]),
                int(a["clock"]),
                int(a["treatment"]),
                as_c_digest(b["input"]),
                as_c_digest(b["profile"]),
                as_c_digest(b["build"]),
                int(b["clock"]),
                int(b["treatment"]),
            )
        )

    def record(self, release_us: int, complete_us: int, overhead_us: int = 0) -> int:
        return int(self.lib.k1_coexist_record_event(release_us, complete_us, overhead_us))

    def admit(
        self,
        *,
        physical_pdm_complete: int = 0,
        useful_u55_admitted: int = 0,
        load_only: int = 1,
        g4_as_unrun: int = 0,
        claimed_p99_pass: int = 0,
        claimed_deadline_us: int = DEADLINE_US,
    ) -> int:
        return int(
            self.lib.k1_coexist_admit(
                physical_pdm_complete,
                useful_u55_admitted,
                load_only,
                g4_as_unrun,
                claimed_p99_pass,
                claimed_deadline_us,
            )
        )

    def raw_events(self) -> list[dict[str, int]]:
        count = int(self.lib.k1_coexist_event_count())
        events = []
        for index in range(count):
            events.append(
                {
                    "index": index,
                    "release_us": int(self.lib.k1_coexist_release_at(index)),
                    "complete_us": int(self.lib.k1_coexist_complete_at(index)),
                    "overhead_us": int(self.lib.k1_coexist_overhead_at(index)),
                    "duration_us": int(self.lib.k1_coexist_duration_at(index)),
                    "missed": int(self.lib.k1_coexist_missed_at(index)),
                }
            )
        return events

    def snapshot(self) -> dict[str, Any]:
        events = self.raw_events()
        durations = [item["duration_us"] for item in events]
        g4 = self.lib.k1_coexist_g4_status_text()
        g6 = self.lib.k1_coexist_g6_text()
        decline = int(self.lib.k1_coexist_last_decline())
        return {
            "identity_set": bool(self.lib.k1_coexist_identity_set()),
            "treatment": int(self.lib.k1_coexist_treatment()),
            "sample_rate_hz": int(self.lib.k1_coexist_sample_rate_hz()),
            "clock_hz": int(self.lib.k1_coexist_clock_hz()),
            "hop_period_us": int(self.lib.k1_coexist_hop_period_us()),
            "deadline_us": int(self.lib.k1_coexist_deadline_us()),
            "event_count": len(events),
            "events": events,
            "durations_us": durations,
            "p50_us": int(self.lib.k1_coexist_percentile(50)) if events else None,
            "p95_us": int(self.lib.k1_coexist_percentile(95)) if events else None,
            "p99_us": int(self.lib.k1_coexist_percentile(99)) if events else None,
            "max_us": int(self.lib.k1_coexist_max_us()) if events else None,
            "deadline_misses": int(self.lib.k1_coexist_deadline_misses()),
            "window_start_us": int(self.lib.k1_coexist_window_start_us()),
            "window_end_us": int(self.lib.k1_coexist_window_end_us()),
            "overhead_sum_us": int(self.lib.k1_coexist_overhead_sum_us()),
            "has_distribution": bool(self.lib.k1_coexist_has_distribution()),
            "p99_scored": bool(self.lib.k1_coexist_p99_scored()),
            "p99_allocation_us": int(self.lib.k1_coexist_p99_allocation_us()),
            "admitted": bool(self.lib.k1_coexist_admitted()),
            "last_decline": decline,
            "last_decline_name": DECLINE[decline],
            "blockers": int(self.lib.k1_coexist_blockers()),
            "physical_not_run": bool(self.lib.k1_coexist_physical_not_run()),
            "g4_status": g4.decode("ascii") if isinstance(g4, bytes) else str(g4),
            "g4_o2_misses": int(self.lib.k1_coexist_g4_o2_misses()),
            "g4_o3_misses": int(self.lib.k1_coexist_g4_o3_misses()),
            "g4_already_failed": bool(self.lib.k1_coexist_g4_already_failed()),
            "g6": g6.decode("ascii") if isinstance(g6, bytes) else str(g6),
        }


def load_host_pair(directory: Path | None = None) -> tuple[HostPair, dict[str, Any]]:
    if directory is None:
        directory = Path(tempfile.mkdtemp(prefix="k1-audio-npu-pair-host-"))
    suffix = "dylib" if sys.platform == "darwin" else "so"
    library_path = directory / f"libcoexist_probe.{suffix}"
    compile_receipt = compile_host_library(library_path)
    return HostPair(CDLL(str(library_path))), compile_receipt


def frozen_pair_identity() -> dict[str, str]:
    hashes = observed_source_hashes()
    return {
        "input": PCM_SHA256,
        "profile": hashes["npu_coexist_profile"],
        "build": LAST_IDENTIFIED_BUILD,
        "clock": SAMPLE_RATE_HZ,
    }


def matching_pair_sides() -> tuple[dict[str, Any], dict[str, Any]]:
    identity = frozen_pair_identity()
    audio = dict(identity, treatment=TREATMENT_AUDIO_ONLY)
    npu = dict(identity, treatment=TREATMENT_USEFUL_NPU)
    return audio, npu


def host_fixture_events(treatment: int) -> list[tuple[int, int, int]]:
    """Synthetic HOST durations. Not a physical measurement."""
    if treatment == TREATMENT_AUDIO_ONLY:
        durations = (4100, 4200, 4300, 4400, 4500, 4600, 4700, 4800)
    else:
        durations = (5100, 5300, 5500, 7000, 7600, 8100, 9000, 9684)
    events = []
    cursor = 1000
    for duration in durations:
        overhead = 40 if treatment == TREATMENT_AUDIO_ONLY else 55
        events.append((cursor, cursor + duration, overhead))
        cursor += HOP_PERIOD_US
    return events


def run_host_treatment(pair: HostPair, treatment: int) -> dict[str, Any]:
    identity = frozen_pair_identity()
    rc = pair.bind(identity["input"], identity["profile"], identity["build"], SAMPLE_RATE_HZ, SAMPLE_RATE_HZ)
    if rc != 0:
        raise PairError("bind_failed", f"k1_coexist_bind returned {rc}")
    if pair.set_treatment(treatment) != 0:
        raise PairError("treatment_failed", f"undeclared treatment {treatment}")
    for release_us, complete_us, overhead_us in host_fixture_events(treatment):
        if pair.record(release_us, complete_us, overhead_us) != 0:
            raise PairError("record_failed", "HOST fixture event rejected")
    snap = pair.snapshot()
    durations = snap["durations_us"]
    if independent_percentile(durations, 50) != snap["p50_us"]:
        raise PairError("aggregation_mismatch", "C p50 does not match independent rank")
    if independent_percentile(durations, 95) != snap["p95_us"]:
        raise PairError("aggregation_mismatch", "C p95 does not match independent rank")
    if independent_percentile(durations, 99) != snap["p99_us"]:
        raise PairError("aggregation_mismatch", "C p99 does not match independent rank")
    if independent_max(durations) != snap["max_us"]:
        raise PairError("aggregation_mismatch", "C max does not match independent max")
    if independent_misses(durations) != snap["deadline_misses"]:
        raise PairError("aggregation_mismatch", "C misses do not match independent misses")
    if snap["p99_scored"]:
        raise PairError("p99_invented", "HOST fixture scored unresolved p99")
    if snap["deadline_us"] != DEADLINE_US:
        raise PairError("deadline_widened", "HOST fixture changed 7500 us")
    return {
        "treatment": treatment,
        "treatment_name": "audio-only" if treatment == TREATMENT_AUDIO_ONLY else "useful-npu-active",
        "qualification_level": "HOST",
        "arm_binary_executed": False,
        "physical_pair": "NOT_RUN",
        "snapshot": snap,
        "independent": {
            "p50_us": independent_percentile(durations, 50),
            "p95_us": independent_percentile(durations, 95),
            "p99_us": independent_percentile(durations, 99),
            "max_us": independent_max(durations),
            "deadline_misses": independent_misses(durations),
        },
    }


def load_predecessor_not_run() -> dict[str, Any]:
    pdm = json.loads(PDM_NOT_RUN.read_text(encoding="utf-8")) if PDM_NOT_RUN.is_file() else {}
    u55 = json.loads(U55_NOT_RUN.read_text(encoding="utf-8")) if U55_NOT_RUN.is_file() else {}
    pdm_status = pdm.get("physical_capture")
    u55_status = u55.get("u55_current_target")
    return {
        "wp13_physical_pdm": pdm_status if pdm_status else "MISSING_RECEIPT",
        "wp14_useful_u55": u55_status if u55_status else "MISSING_RECEIPT",
        "wp13_receipt": str(PDM_NOT_RUN) if PDM_NOT_RUN.is_file() else None,
        "wp14_receipt": str(U55_NOT_RUN) if U55_NOT_RUN.is_file() else None,
        "wp13_level": pdm.get("qualification_level"),
        "wp14_level": u55.get("qualification_level"),
        "physical_pdm_complete": pdm_status == "PASS",
        "useful_u55_admitted": u55_status == "PASS" and u55.get("claims_useful_inference") is True,
        "both_not_run": pdm_status == "NOT_RUN" and u55_status == "NOT_RUN",
        "named_missing_facts": list(NAMED_PHYSICAL_FACTS),
        "blocked_until": list(PHYSICAL_BLOCKERS),
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


def missing_named_physical_facts(identities: Mapping[str, Any] | None, predecessors: Mapping[str, Any] | None) -> list[str]:
    identities = identities or {}
    predecessors = predecessors or {}
    ownership = identities.get("ownership_record") if isinstance(identities.get("ownership_record"), Mapping) else {}
    missing: list[str] = []
    live_uid = identities.get("target_uid") if identities.get("live_target") == "VERIFIED" else None
    if not live_uid:
        missing.append("live_uid")
    if not (isinstance(ownership, Mapping) and ownership.get("exclusive_owner")):
        missing.append("exclusive_owner")
    if not predecessors.get("useful_u55_admitted"):
        missing.append("useful_u55_admission")
    if not predecessors.get("physical_pdm_complete"):
        missing.append("physical_pdm")
    return missing


def make_receipt(
    *,
    qualification_level: str,
    pair: Mapping[str, Any],
    identities: Mapping[str, Any] | None = None,
    execution: Mapping[str, Any] | None = None,
    tools: list[dict[str, Any]] | None = None,
    limitations: list[str] | None = None,
    receipt_id: str = "wp15-audio-npu-pair",
    claims_physical_pair: bool = False,
    p99_pass_fail: str = "unscored",
    claimed_deadline_us: int = DEADLINE_US,
    g4_status: str = G4,
) -> dict[str, Any]:
    if qualification_level not in LEVELS:
        raise PairError("qualification_level", f"Unknown {qualification_level}.")
    identities = dict(identities or {})
    execution = dict(execution or {})
    pair_payload = dict(pair)
    errors: list[dict[str, str]] = []
    blocked = ["audio-npu-physical-pair", "p99-pass-fail"]
    predecessors = pair_payload.get("predecessors") or load_predecessor_not_run()

    audio = pair_payload.get("audio") if isinstance(pair_payload.get("audio"), Mapping) else None
    npu = pair_payload.get("npu") if isinstance(pair_payload.get("npu"), Mapping) else None
    comparable = pair_payload.get("comparable")
    if audio and npu:
        for field, code in (
            ("input", "pair-mismatch-input"),
            ("profile", "pair-mismatch-profile"),
            ("build", "pair-mismatch-build"),
            ("clock", "pair-mismatch-clock"),
        ):
            if audio.get(field) != npu.get(field):
                errors.append({"code": code, "message": field})
        treatments = {audio.get("treatment"), npu.get("treatment")}
        if treatments != {TREATMENT_AUDIO_ONLY, TREATMENT_USEFUL_NPU}:
            errors.append({"code": "pair-treatment-not-declared", "message": "pair requires audio-only vs useful-npu-active"})
    if comparable not in (None, 0, "OK") and comparable not in (DECLINE[0],):
        pass

    load_only = execution.get("load_only") is True or str(execution.get("npu_role") or "").lower() in {
        "load-only",
        "identified-platform-load-only",
    }
    constant = list(pair_payload.get("expected_raw_output") or []) == LOAD_ONLY_OUTPUT
    useful_treatment = (
        execution.get("useful_npu_treatment") is True
        or pair_payload.get("useful_npu_treatment") is True
        or (npu is not None and npu.get("treatment") == TREATMENT_USEFUL_NPU and pair_payload.get("claims_useful_inference") is True)
    )
    if (load_only or constant) and (useful_treatment or pair_payload.get("claims_useful_inference") is True):
        errors.append(
            {
                "code": "load-only-as-useful-npu-treatment",
                "message": "Load-only NPU graph cannot satisfy useful-NPU-active treatment",
            }
        )
    if claimed_deadline_us != DEADLINE_US:
        errors.append({"code": "deadline-widened", "message": f"claimed {claimed_deadline_us} authorised {DEADLINE_US}"})
    if p99_pass_fail in {"PASS", "FAIL", "pass", "fail"}:
        errors.append({"code": "p99-invented-pass", "message": "6000 us p99 remains unscored"})
    if g4_status in {"UNRUN", "NOT_RUN", "unrun"}:
        errors.append({"code": "g4-rerun-as-unrun", "message": "G4 O2/O3 already FAILED; do not relabel unrun"})
    if g4_status not in (None, G4) and g4_status not in {"FAILED"}:
        if not pair_payload.get("g4_new_authority"):
            errors.append({"code": "g4-already-failed-relabelled", "message": "G4 stays FAILED with 2005/2006 misses"})
    if pair_payload.get("claims_distribution") is True and not pair_payload.get("raw_events"):
        errors.append({"code": "missing-raw-events", "message": "Distributions require raw event records"})
    if claims_physical_pair and qualification_level == "HOST":
        errors.append({"code": "host-events-as-physical-pair", "message": "HOST synthetic events cannot pass the physical pair"})
    if claims_physical_pair and load_only:
        errors.append(
            {
                "code": "load-only-as-useful-npu-treatment",
                "message": "Load-only NPU graph cannot satisfy the physical pair",
            }
        )

    if qualification_level == "CURRENT_TARGET":
        for field in missing_current_target_fields(identities):
            errors.append({"code": "current-target-identity-missing", "message": field})
        for fact in missing_named_physical_facts(identities, predecessors):
            errors.append({"code": "physical-pair-missing-fact", "message": fact})
        if execution.get("arm_binary_executed") is not True:
            errors.append({"code": "physical-pair-not-executed", "message": "CURRENT_TARGET pair requires identified-target execution"})
        ownership = identities.get("ownership_record")
        if isinstance(ownership, Mapping) and ownership.get("method") == "port-name":
            errors.append({"code": "port-name-is-not-identity", "message": "USB/serial port names are not board identity"})
        if execution.get("compiler_facts_only"):
            errors.append({"code": "compiler-facts-as-current-target", "message": "Build-only facts cannot claim CURRENT_TARGET pair"})
        if not predecessors.get("useful_u55_admitted"):
            errors.append({"code": "useful-u55-not-admitted", "message": "WP14 useful-U55 CURRENT_TARGET is NOT_RUN"})
        if not predecessors.get("physical_pdm_complete"):
            errors.append({"code": "physical-pdm-not-run", "message": "WP13 physical PDM is NOT_RUN"})

    status = "PASS" if not errors else "FAIL"
    if qualification_level == "CURRENT_TARGET" and errors:
        status = "FAIL_CLOSED"
    return {
        "schema_version": 2,
        "receipt_id": receipt_id,
        "recipe": {"id": "audio-npu-pair", "version": "1"},
        "contract_version": "RA8P1-SOL-1.0.0",
        "qualification_level": qualification_level,
        "execution": execution,
        "identities": identities,
        "pair": pair_payload,
        "claims_physical_pair": claims_physical_pair,
        "p99_pass_fail": "unscored",
        "p99_us": P99_US,
        "authorised_deadline_us": DEADLINE_US,
        "g4_status": G4,
        "g4_o2_deadline_misses": G4_O2_MISSES,
        "g4_o3_deadline_misses": G4_O3_MISSES,
        "g6": G6,
        "predecessors": predecessors,
        "tools": tools or [],
        "errors": errors,
        "blocked_cells": blocked,
        "physical_pair": "NOT_RUN",
        "ok": not errors,
        "status": status,
        "limitations": limitations
        or [
            "Load-only graph cannot satisfy useful-NPU treatment.",
            "WP13 physical PDM is NOT_RUN.",
            "WP14 useful U55 CURRENT_TARGET is NOT_RUN.",
            "Physical pair CURRENT_TARGET is NOT_RUN.",
            "G4 O2/O3 remain FAILED (2005/2006 misses); not unrun.",
            "6000 us p99 is unscored. 7500 us is not widened.",
        ],
        "started_at": datetime.now(timezone.utc).isoformat(),
        "completed_at": datetime.now(timezone.utc).isoformat(),
    }


def current_target_not_run(pair: Mapping[str, Any] | None = None) -> dict[str, Any]:
    predecessors = load_predecessor_not_run()
    payload = dict(pair or {})
    payload["predecessors"] = predecessors
    payload.setdefault("expected_raw_output", LOAD_ONLY_OUTPUT)
    return make_receipt(
        qualification_level="HOST",
        receipt_id="wp15-audio-npu-pair-current-target-not-run",
        identities={
            "last_identified_uid": LAST_IDENTIFIED_UID,
            "last_identified_build_id": LAST_IDENTIFIED_BUILD,
            "last_identified_level": "HISTORICAL_TARGET",
            "live_target": "NOT_VERIFIED",
            "live_loaded_image": "unknown-no-reenumeration",
            "named_missing_facts": list(NAMED_PHYSICAL_FACTS),
        },
        execution={
            "arm_binary_executed": False,
            "hardware_session": False,
            "load_only": True,
            "npu_role": "identified-platform-load-only",
            "useful_npu_treatment": False,
        },
        pair=payload,
        claims_physical_pair=False,
        limitations=[
            "live_target is NOT_VERIFIED; last UID is HISTORICAL_TARGET.",
            "CURRENT_TARGET physical pair is NOT_RUN because WP13 physical PDM and WP14 useful-U55 are both NOT_RUN.",
            "Named missing facts: live UID, exclusive owner, useful-U55 admission, physical PDM.",
            "No USB, flash, serial, audio, cadence runner, or song loop.",
            "G4 O2/O3 remain FAILED. 6000 us p99 unscored. 7500 us not widened.",
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
        "cannot_satisfy": "physical-audio-npu-pair",
        "physical_pair": "NOT_RUN",
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
    adapter = build_adapter_document()
    write_json(output / "adapter-k1-ra8p1-audio-npu-pair-v1.json", adapter)
    predecessors = load_predecessor_not_run()
    pair_lib, host_compile = load_host_pair()
    audio_side, npu_side = matching_pair_sides()
    comparable = pair_lib.pair_comparable(audio_side, npu_side)
    probe_rc = int(pair_lib.lib.k1_coexist_probe())
    load_rc = pair_lib.admit(load_only=1, useful_u55_admitted=0, physical_pdm_complete=0)
    acquire_rc = int(pair_lib.lib.k1_coexist_acquire())
    audio = run_host_treatment(pair_lib, TREATMENT_AUDIO_ONLY)
    npu = run_host_treatment(pair_lib, TREATMENT_USEFUL_NPU)
    pair_payload = {
        "audio": audio_side,
        "npu": npu_side,
        "comparable": DECLINE[comparable],
        "audio_host": audio,
        "npu_host": npu,
        "predecessors": predecessors,
        "expected_raw_output": LOAD_ONLY_OUTPUT,
        "claims_useful_inference": False,
        "raw_events": {
            "audio-only": audio["snapshot"]["events"],
            "useful-npu-active": npu["snapshot"]["events"],
        },
        "probe_decline": DECLINE[probe_rc],
        "load_only_decline": DECLINE[load_rc],
        "acquire_rc": acquire_rc,
    }
    host_receipt = make_receipt(
        qualification_level="HOST",
        receipt_id="wp15-audio-npu-pair-host",
        identities={"adapter_id": ADAPTER_ID, "source_sha256": sha256_file(SOURCE_C)},
        execution={
            "load_only": True,
            "npu_role": "identified-platform-load-only",
            "arm_binary_executed": False,
            "useful_npu_treatment": False,
        },
        pair=pair_payload,
        claims_physical_pair=False,
        tools=[{"name": "cc", "argv": host_compile["command"], "exit_code": 0, "sha256": host_compile["sha256"]}],
    )
    write_json(output / "audio-npu-pair-host-receipt.json", host_receipt)
    with tempfile.TemporaryDirectory(prefix="k1-audio-npu-pair-cross-") as temp:
        cross = cross_compile(Path(temp) / "coexist_probe.o")
        if cross.get("object"):
            dest = output / "coexist_probe.cross.o"
            dest.write_bytes(Path(cross["object"]).read_bytes())
            cross["object"] = str(dest)
            cross["object_sha256"] = sha256_file(dest)
        write_json(output / "audio-npu-pair-cross-compiled-receipt.json", cross)
    physical = current_target_not_run(pair_payload)
    write_json(output / "audio-npu-pair-current-target-not-run.json", physical)
    g4_reuse = {
        "schema_version": 2,
        "receipt_id": "wp15-g4-already-failed",
        "qualification_level": "HISTORICAL_TARGET",
        "g4_status": G4,
        "o2_deadline_misses": G4_O2_MISSES,
        "o3_deadline_misses": G4_O3_MISSES,
        "do_not_repeat_as_unrun": True,
        "do_not_widen_deadline_us": DEADLINE_US,
        "p99_us": P99_US,
        "p99_status": "unscored",
        "note": "G4 O2/O3 already FAILED. WP15 reuses those receipts and does not re-run the cells.",
    }
    write_json(output / "g4-already-failed-reuse.json", g4_reuse)
    print(
        json.dumps(
            {
                "host": host_receipt["status"],
                "comparable": DECLINE[comparable],
                "probe": DECLINE[probe_rc],
                "load_only": DECLINE[load_rc],
                "physical_pair": physical["physical_pair"],
                "predecessors": {
                    "pdm": predecessors["wp13_physical_pdm"],
                    "u55": predecessors["wp14_useful_u55"],
                },
                "g4": G4,
                "p99": "unscored",
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
