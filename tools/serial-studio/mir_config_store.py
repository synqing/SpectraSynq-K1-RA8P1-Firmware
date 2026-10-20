#!/usr/bin/env python3
"""M5 — Full configuration save/load on host or mock storage only. No Titan NVM."""
from __future__ import annotations

import json
import os
import tempfile
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
PERSISTENCE_SCOPE = "host_mock"


class ConfigStoreError(Exception):
    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code
        self.message = message


def _validate_record(record: dict[str, Any]) -> dict[str, Any]:
    if not isinstance(record, dict):
        raise ConfigStoreError("corruption", "record is not an object")
    if record.get("persistence_scope") != PERSISTENCE_SCOPE:
        raise ConfigStoreError("scope", "live NVM scope is NOT AUTHORIZED; host_mock only")
    version = record.get("schema_version")
    if version is None:
        raise ConfigStoreError("partial_record", "missing schema_version")
    if int(version) != SCHEMA_VERSION:
        raise ConfigStoreError("version_mismatch", f"unsupported schema_version {version}")
    if "config" not in record or not isinstance(record["config"], dict):
        raise ConfigStoreError("partial_record", "missing config object")
    # Unknown fields are retained but must not silently promote scope.
    return record


class HostConfigStore:
    def __init__(self, root: Path) -> None:
        self.root = Path(root)
        self.root.mkdir(parents=True, exist_ok=True)

    def save(self, name: str, config: dict[str, Any], *, extra: dict[str, Any] | None = None) -> Path:
        record = {
            "schema_version": SCHEMA_VERSION,
            "persistence_scope": PERSISTENCE_SCOPE,
            "live_nvm": "NOT_AUTHORIZED",
            "config": dict(config),
        }
        if extra:
            for key, value in extra.items():
                if key in ("schema_version", "persistence_scope", "config", "live_nvm"):
                    continue
                record[key] = value
        path = self.root / f"{name}.json"
        # Atomic write: temp then replace. Interrupted write leaves old file intact.
        fd, tmp_name = tempfile.mkstemp(prefix=f".{name}.", suffix=".tmp", dir=self.root)
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as fh:
                json.dump(record, fh, indent=2, sort_keys=True)
                fh.write("\n")
                fh.flush()
                os.fsync(fh.fileno())
            Path(tmp_name).replace(path)
        except Exception:
            try:
                Path(tmp_name).unlink(missing_ok=True)
            except OSError:
                pass
            raise
        return path

    def load(self, name: str) -> dict[str, Any]:
        path = self.root / f"{name}.json"
        try:
            text = path.read_text(encoding="utf-8")
        except FileNotFoundError as exc:
            raise ConfigStoreError("missing", str(exc)) from exc
        try:
            record = json.loads(text)
        except json.JSONDecodeError as exc:
            raise ConfigStoreError("corruption", str(exc)) from exc
        return _validate_record(record)

    def round_trip(self, name: str, config: dict[str, Any]) -> dict[str, Any]:
        self.save(name, config)
        return self.load(name)["config"]
