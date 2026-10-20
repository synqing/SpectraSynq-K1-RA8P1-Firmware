#!/usr/bin/env python3
"""M4 — Host control surface against a mock endpoint. No target / CDC / real broker."""
from __future__ import annotations

import copy
import threading
import time
from typing import Any


class ControlError(Exception):
    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code
        self.message = message


class MockControlEndpoint:
    """Atomic revision-checked configuration store used by host tests."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._revision = 1
        self._cfg: dict[str, Any] = {
            "version": 2,
            "revision": 1,
            "palette_a": 0,
            "palette_b": 1,
            "mode_a": 32,
            "mode_b": 0,
            "emit_on": 0,
            "brightness": 24,
            "flags": 1,
        }
        self._connected = True
        self._seq = 0
        self._last_seq_applied = 0

    def disconnect(self) -> None:
        self._connected = False

    def reconnect(self) -> None:
        self._connected = True

    def readback(self) -> dict[str, Any]:
        if not self._connected:
            raise ControlError("disconnect", "endpoint disconnected")
        with self._lock:
            return copy.deepcopy(self._cfg)

    def apply(
        self,
        desired: dict[str, Any],
        *,
        expected_revision: int | None = None,
        sequence: int | None = None,
        malformed: bool = False,
    ) -> dict[str, Any]:
        if not self._connected:
            raise ControlError("disconnect", "endpoint disconnected")
        if malformed:
            raise ControlError("malformed", "malformed command rejected")
        if sequence is not None:
            if sequence <= self._last_seq_applied:
                raise ControlError("reordered", f"sequence {sequence} <= last {self._last_seq_applied}")
        with self._lock:
            current_rev = int(self._cfg["revision"])
            if expected_revision is None:
                raise ControlError("missing_revision", "revision check required")
            if expected_revision != current_rev:
                raise ControlError(
                    "stale_revision",
                    f"expected revision {expected_revision}, have {current_rev}",
                )
            # Atomic: build next state then swap once.
            next_cfg = copy.deepcopy(self._cfg)
            for key, value in desired.items():
                if key in ("revision", "version"):
                    continue
                next_cfg[key] = value
            next_cfg["emit_on"] = 0  # host campaign never enables emit
            next_cfg["revision"] = current_rev + 1
            self._cfg = next_cfg
            self._revision = next_cfg["revision"]
            if sequence is not None:
                self._last_seq_applied = sequence
            else:
                self._seq += 1
                self._last_seq_applied = self._seq
            return copy.deepcopy(self._cfg)


class HostControlClient:
    """Client that always revision-checks and readbacks after mutate."""

    def __init__(self, endpoint: MockControlEndpoint) -> None:
        self.endpoint = endpoint

    def change(self, patch: dict[str, Any], *, sequence: int | None = None) -> dict[str, Any]:
        current = self.endpoint.readback()
        applied = self.endpoint.apply(
            patch,
            expected_revision=int(current["revision"]),
            sequence=sequence,
        )
        readback = self.endpoint.readback()
        if readback["revision"] != applied["revision"]:
            raise ControlError("readback_mismatch", "applied revision did not stick")
        return readback
