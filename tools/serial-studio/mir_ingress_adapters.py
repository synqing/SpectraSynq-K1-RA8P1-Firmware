#!/usr/bin/env python3
"""M7 — Host ingress adapters under shared source/epoch contract. No fidelity claim."""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


class IngressError(Exception):
    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code
        self.message = message


@dataclass
class SourceEpoch:
    source_id: str
    epoch: int

    def matches(self, other: "SourceEpoch") -> bool:
        return self.source_id == other.source_id and self.epoch == other.epoch


@dataclass
class IngressFrame:
    source_id: str
    epoch: int
    sequence: int
    samples: list[int] = field(default_factory=list)
    availability_time_ms: int = 0


class HostIngressAdapter:
    """Accepts host-side frames only when source/epoch match the bound contract."""

    def __init__(self, bound: SourceEpoch) -> None:
        self.bound = bound
        self._last_seq = -1
        self.accepted: list[IngressFrame] = []
        self.rejected: list[dict[str, Any]] = []

    def rebind(self, bound: SourceEpoch) -> None:
        self.bound = bound
        self._last_seq = -1

    def admit(self, frame: IngressFrame) -> IngressFrame:
        if frame.source_id != self.bound.source_id:
            self.rejected.append({"code": "source_mismatch", "frame_seq": frame.sequence})
            raise IngressError("source_mismatch", "frame source_id does not match bound source")
        if frame.epoch != self.bound.epoch:
            self.rejected.append({"code": "stale_epoch", "frame_seq": frame.sequence})
            raise IngressError("stale_epoch", "frame epoch does not match bound epoch")
        if frame.sequence <= self._last_seq:
            self.rejected.append({"code": "reordered", "frame_seq": frame.sequence})
            raise IngressError("reordered", "frame sequence not strictly increasing")
        self._last_seq = frame.sequence
        self.accepted.append(frame)
        return frame

    def note(self) -> dict[str, Any]:
        return {
            "fidelity_claim": False,
            "bound_source": self.bound.source_id,
            "bound_epoch": self.bound.epoch,
            "accepted": len(self.accepted),
            "rejected": len(self.rejected),
            "claim": "host contract exercise only; not physical ingress fidelity",
        }
