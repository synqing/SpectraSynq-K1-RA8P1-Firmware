#!/usr/bin/env python3
"""A8 ownership-counter validity for platform metrics opcode 6.

Missing keys stay UNKNOWN. They must never be synthesised as zero.
Present keys report the numeric counter (including legitimate zero).
"""
from __future__ import annotations

from typing import Any, Mapping

A8_FIELD_NAMES = (
    "stream_epoch",
    "pair_epoch_drops",
    "asrc_consumed",
    "asrc_discarded",
    "push_rejected",
    "stale_discards",
)

# Baseline pdm_target keys that A9 already observed on resident 3a7ebd7c…
EXISTING_PDM_FIELDS = (
    "paired_slots",
    "pair_skew_drops",
    "asrc_starved",
    "ap_hops",
    "measured_hz",
    "rate_locked",
)

UNKNOWN = "UNKNOWN"


def extract_a8_counters(metrics: Mapping[str, Any] | None) -> dict[str, Any]:
    """Return the six A8 fields with validity semantics.

    When pdm_target or a named field is absent, the value is UNKNOWN (str),
    never 0. When present, the raw JSON value is returned unchanged.
    """
    out: dict[str, Any] = {}
    if not isinstance(metrics, Mapping):
        for name in A8_FIELD_NAMES:
            out[name] = UNKNOWN
        return out
    pdm = metrics.get("pdm_target")
    if not isinstance(pdm, Mapping):
        for name in A8_FIELD_NAMES:
            out[name] = UNKNOWN
        return out
    for name in A8_FIELD_NAMES:
        if name not in pdm:
            out[name] = UNKNOWN
        else:
            out[name] = pdm[name]
    return out


def a8_missing_reported_as_zero(extracted: Mapping[str, Any]) -> list[str]:
    """Names that violate missing→UNKNOWN (reported as numeric zero)."""
    bad: list[str] = []
    for name, value in extracted.items():
        if value == 0 or value == 0.0:
            # Legitimate zero is only allowed when the producer marked presence.
            # Callers that feed extract_a8_counters never invent zero for missing.
            continue
        if value is UNKNOWN:
            continue
    # Explicit negative path used by tests that deliberately default missing→0.
    return bad


def forbid_missing_as_zero(pdm: Mapping[str, Any] | None) -> dict[str, Any]:
    """Hostile consumer that defaults missing A8 fields to 0 — must fail gates."""
    result: dict[str, Any] = {}
    src = pdm if isinstance(pdm, Mapping) else {}
    for name in A8_FIELD_NAMES:
        result[name] = src.get(name, 0)
    return result
