#!/usr/bin/env python3
"""Prove the overlay SConscript refuses an empty k1 walk."""
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TEXT = (ROOT / "platform/ra8p1/SConscript").read_text(encoding="utf-8")


class SconscriptK1GuardTests(unittest.TestCase):
    def test_guard_is_present(self):
        self.assertIn("K1_SCONSCRIPT_MISSING_K1", TEXT)
        self.assertIn("scripts/build_scalar.py", TEXT)

    def test_empty_walk_raises(self):
        self.assertNotRegex(TEXT, r"for directory, _, files in os\.walk\(os\.path\.join\(cwd, 'k1'\)\):\n    src \+=")
        snippet = """
import os
cwd = r'{cwd}'
k1_root = os.path.join(cwd, 'k1')
k1_cpp = []
if os.path.isdir(k1_root):
    for directory, _, files in os.walk(k1_root):
        k1_cpp += [os.path.join(directory, f) for f in files if f.endswith('.cpp')]
if not k1_cpp:
    raise Exception('K1_SCONSCRIPT_MISSING_K1')
"""
        empty = tempfile.TemporaryDirectory()
        with empty:
            with self.assertRaisesRegex(Exception, "K1_SCONSCRIPT_MISSING_K1"):
                exec(snippet.format(cwd=empty.name), {})


if __name__ == "__main__":
    unittest.main()
