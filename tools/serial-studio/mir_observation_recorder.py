#!/usr/bin/env python3
"""M2 — Bounded observation recording: rotation, max size, gap markers, write-failure detection.

Host/mock only. Does not open CDC or the real broker.
"""
from __future__ import annotations

import json
import queue
import threading
import time
from pathlib import Path
from typing import Any


class BoundedObservationRecorder:
    """Append-only NDJSON recorder with hard bounds and explicit gap markers."""

    def __init__(
        self,
        path: Path,
        *,
        max_items: int = 20000,
        max_bytes: int = 32 * 1024 * 1024,
        gap_threshold_s: float = 1.0,
    ) -> None:
        self.path = Path(path)
        self.max_items = max_items
        self.max_bytes = max_bytes
        self.gap_threshold_s = gap_threshold_s
        self.q: queue.Queue = queue.Queue(max_items)
        self.dropped = 0
        self.high = 0
        self.failed = False
        self.write_failures = 0
        self.rotations = 0
        self.gaps = 0
        self._last_mono: float | None = None
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self._fp = self.path.open("a", encoding="utf-8")
        self._lock = threading.Lock()
        self._stop = False
        self._thr = threading.Thread(target=self._drain, daemon=True)
        self._thr.start()

    def put(self, item: dict[str, Any]) -> None:
        now = time.monotonic()
        if self._last_mono is not None and (now - self._last_mono) > self.gap_threshold_s:
            self.mark_gap(
                reason="time_discontinuity",
                gap_s=round(now - self._last_mono, 6),
            )
        self._last_mono = now
        payload = dict(item)
        payload.setdefault("t", now)
        payload.setdefault("kind", "observation")
        try:
            self.q.put_nowait(payload)
            self.high = max(self.high, self.q.qsize())
        except queue.Full:
            self.dropped += 1
            self.failed = True
            self._sync_gap(
                {
                    "kind": "gap",
                    "gap_reason": "queue_full",
                    "dropped_total": self.dropped,
                    "t": now,
                }
            )

    def mark_gap(self, *, reason: str, **extra: Any) -> None:
        row = {"kind": "gap", "gap_reason": reason, "t": time.monotonic(), **extra}
        try:
            self.q.put_nowait(row)
            self.gaps += 1
        except queue.Full:
            self._sync_gap(row)

    def _sync_gap(self, row: dict[str, Any]) -> None:
        """Best-effort gap write when the async queue cannot accept more items."""
        with self._lock:
            try:
                self._rotate_if_needed_locked()
                self._fp.write(json.dumps(row) + "\n")
                self._fp.flush()
                self.gaps += 1
            except OSError:
                self.failed = True
                self.write_failures += 1

    def _rotate_if_needed_locked(self) -> None:
        try:
            size = self._fp.tell()
        except OSError:
            return
        if size < self.max_bytes:
            return
        self._fp.close()
        stamp = time.strftime("%Y%m%dT%H%M%S")
        rotated = self.path.with_name(self.path.name + "." + stamp)
        self.path.replace(rotated)
        self._fp = self.path.open("a", encoding="utf-8")
        self.rotations += 1
        self._fp.write(
            json.dumps(
                {
                    "kind": "gap",
                    "gap_reason": "rotation",
                    "rotated_to": str(rotated.name),
                    "t": time.monotonic(),
                }
            )
            + "\n"
        )
        self.gaps += 1

    def _drain(self) -> None:
        while not self._stop:
            try:
                item = self.q.get(timeout=0.2)
            except queue.Empty:
                continue
            with self._lock:
                try:
                    self._rotate_if_needed_locked()
                    self._fp.write(json.dumps(item) + "\n")
                    self._fp.flush()
                except OSError:
                    self.failed = True
                    self.write_failures += 1
                    self.dropped += 1

    def close(self) -> None:
        # Drain remaining queued items before stopping the worker.
        deadline = time.monotonic() + 2.0
        while time.monotonic() < deadline and not self.q.empty():
            time.sleep(0.05)
        self._stop = True
        self._thr.join(timeout=2)
        # Final synchronous drain for anything left.
        while True:
            try:
                item = self.q.get_nowait()
            except queue.Empty:
                break
            with self._lock:
                try:
                    self._rotate_if_needed_locked()
                    self._fp.write(json.dumps(item) + "\n")
                    self._fp.flush()
                except OSError:
                    self.failed = True
                    self.write_failures += 1
                    self.dropped += 1
        with self._lock:
            try:
                self._fp.close()
            except OSError:
                pass

    def stats(self) -> dict[str, Any]:
        return {
            "path": str(self.path),
            "dropped": self.dropped,
            "high_water": self.high,
            "failed": self.failed,
            "write_failures": self.write_failures,
            "rotations": self.rotations,
            "gaps": self.gaps,
            "max_items": self.max_items,
            "max_bytes": self.max_bytes,
        }
