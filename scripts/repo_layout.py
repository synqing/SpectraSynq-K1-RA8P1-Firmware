#!/usr/bin/env python3
"""Resolve the canonical git checkout when running inside a nested .worktrees path."""
from __future__ import annotations

import subprocess
from pathlib import Path


def canonical_checkout(start: Path) -> Path:
    start = Path(start).resolve()
    try:
        common = subprocess.check_output(
            ["git", "-C", str(start), "rev-parse", "--path-format=absolute", "--git-common-dir"],
            text=True,
        ).strip()
    except subprocess.CalledProcessError:
        return start
    git_dir = Path(common)
    if git_dir.name == ".git":
        return git_dir.parent
    return start


def sibling(start: Path, name: str) -> Path:
    return canonical_checkout(start).parent / name
