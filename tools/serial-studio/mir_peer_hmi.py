#!/usr/bin/env python3
"""M8 — Peer/HMI contract against a mock peer only."""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


class PeerHmiError(Exception):
    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code
        self.message = message


CONTRACT_VERSION = 1


@dataclass
class PeerMessage:
    version: int
    owner: str
    sequence: int
    kind: str
    payload: dict[str, Any] = field(default_factory=dict)


class MockPeerHmi:
    """Emulator peer for version/owner/sequence/stale/disconnect/handoff tests."""

    def __init__(self, owner: str = "mir-host") -> None:
        self.owner = owner
        self.version = CONTRACT_VERSION
        self._connected = True
        self._last_seq = 0
        self._handoff_owner: str | None = None
        self.log: list[dict[str, Any]] = []

    def disconnect(self) -> None:
        self._connected = False
        self.log.append({"event": "disconnect"})

    def reconnect(self) -> None:
        self._connected = True
        self.log.append({"event": "reconnect"})

    def handoff(self, new_owner: str) -> None:
        if not self._connected:
            raise PeerHmiError("disconnect", "cannot handoff while disconnected")
        self._handoff_owner = new_owner
        self.owner = new_owner
        self.log.append({"event": "handoff", "owner": new_owner})

    def accept(self, msg: PeerMessage) -> PeerMessage:
        if not self._connected:
            raise PeerHmiError("disconnect", "peer disconnected")
        if msg.version != self.version:
            raise PeerHmiError("version", f"unsupported version {msg.version}")
        if not msg.owner:
            raise PeerHmiError("malformed", "owner required")
        if msg.kind not in {"observe", "control", "handoff", "heartbeat"}:
            raise PeerHmiError("malformed", f"unknown kind {msg.kind}")
        if msg.sequence <= self._last_seq:
            raise PeerHmiError("stale_or_reordered", f"sequence {msg.sequence} <= {self._last_seq}")
        if msg.owner != self.owner and msg.kind != "handoff":
            raise PeerHmiError("owner", f"owner {msg.owner} is not current owner {self.owner}")
        self._last_seq = msg.sequence
        self.log.append({"event": "accept", "sequence": msg.sequence, "kind": msg.kind})
        if msg.kind == "handoff":
            self.handoff(str(msg.payload.get("new_owner") or msg.owner))
        return msg

    def contract(self) -> dict[str, Any]:
        return {
            "version": self.version,
            "owner": self.owner,
            "sequence_policy": "strictly_increasing",
            "stale_reordered": "reject",
            "disconnect": "reject until reconnect",
            "handoff": "explicit handoff message or handoff()",
            "malformed": "reject",
            "mock_only": True,
            "no_multi_device_claim": True,
        }
