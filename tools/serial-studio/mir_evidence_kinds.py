#!/usr/bin/env python3
"""M3 — Keep replayed observations distinct from recomputed PCM-derived features."""
from __future__ import annotations

from typing import Any

EVIDENCE_OBSERVATION_REPLAY = "OBSERVATION_REPLAY"
EVIDENCE_PCM_RECOMPUTE = "PCM_RECOMPUTE"
EVIDENCE_DIGITAL_FIXTURE = "DIGITAL_FIXTURE"
EVIDENCE_LIVE_OBSERVATION = "LIVE_OBSERVATION"

ALLOWED_KINDS = frozenset(
    {
        EVIDENCE_OBSERVATION_REPLAY,
        EVIDENCE_PCM_RECOMPUTE,
        EVIDENCE_DIGITAL_FIXTURE,
        EVIDENCE_LIVE_OBSERVATION,
    }
)

# These two must never share an evidence_kind label or be merged as one claim.
INCOMPATIBLE_PAIR = (EVIDENCE_OBSERVATION_REPLAY, EVIDENCE_PCM_RECOMPUTE)


class EvidenceKindError(ValueError):
    pass


def require_kind(kind: str) -> str:
    if kind not in ALLOWED_KINDS:
        raise EvidenceKindError(f"unknown evidence kind: {kind}")
    return kind


def label_payload(kind: str, payload: dict[str, Any]) -> dict[str, Any]:
    """Return a copy with an explicit evidence_kind. Never invent a default."""
    require_kind(kind)
    out = dict(payload)
    existing = out.get("evidence_kind")
    if existing is not None and existing != kind:
        raise EvidenceKindError(
            f"refusing to relabel {existing} as {kind}; observation replay and PCM recompute stay distinct"
        )
    out["evidence_kind"] = kind
    out["evidence_kind_note"] = {
        EVIDENCE_OBSERVATION_REPLAY: "Recorded MIR/events/timing/config history only; not audio reanalysis",
        EVIDENCE_PCM_RECOMPUTE: "Deterministic LiveAudioRuntime consume of frozen PCM; not a live observation session",
        EVIDENCE_DIGITAL_FIXTURE: "Synthetic or silence digital PCM; not named real music",
        EVIDENCE_LIVE_OBSERVATION: "Live target observation; host tests do not prove photons",
    }[kind]
    return out


def assert_not_merged(a: dict[str, Any], b: dict[str, Any]) -> None:
    """Reject treating observation replay and PCM recompute as the same evidence."""
    ka = a.get("evidence_kind")
    kb = b.get("evidence_kind")
    if ka is None or kb is None:
        raise EvidenceKindError("both payloads require evidence_kind before comparison")
    pair = {ka, kb}
    if pair == set(INCOMPATIBLE_PAIR):
        raise EvidenceKindError(
            "OBSERVATION_REPLAY and PCM_RECOMPUTE must never be labelled as the same evidence"
        )


def same_claim_allowed(a: dict[str, Any], b: dict[str, Any]) -> bool:
    try:
        assert_not_merged(a, b)
    except EvidenceKindError:
        return False
    return a.get("evidence_kind") == b.get("evidence_kind")
