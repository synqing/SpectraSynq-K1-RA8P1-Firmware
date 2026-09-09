"""Fault battery uses disposable destinations; the oracle is never mutated."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("verifier", ROOT / "scripts/verify_imports.py")
verifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verifier)


class ImportAcceptance(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "docs").mkdir()
        shutil.copyfile(ROOT / "docs/IMPORT-MANIFEST.tsv", self.root / "docs/IMPORT-MANIFEST.tsv")
        self.name = "core/audio/media_time.h"
        self.required([self.name])
        self.destination = self.root / "src/k1" / self.name
        self.destination.parent.mkdir(parents=True)
        shutil.copyfile(ROOT / "src/k1" / self.name, self.destination)

    def required(self, paths):
        (self.root / "docs/import-slices.json").write_text(json.dumps({"probe": paths}))

    def check(self):
        return verifier.verify(self.root, verifier.REFERENCE, "probe", True)

    def test_valid_subset_not_all_future_imports(self):
        result = self.check()
        self.assertTrue(result["pass"])
        self.assertEqual((result["required"], result["checked"]), (1, 1))

    def test_empty_rejected(self):
        self.required([])
        with self.assertRaisesRegex(ValueError, "empty"):
            self.check()

    def test_deletion_after_acceptance_rejected(self):
        self.assertTrue(self.check()["pass"])
        self.destination.unlink()
        result = self.check()
        self.assertFalse(result["pass"])
        self.assertEqual(result["missing"], 1)

    def test_wrong_byte_rejected(self):
        self.destination.write_bytes(b"wrong")
        result = self.check()
        self.assertFalse(result["pass"])
        self.assertEqual(result["divergent"], 1)

    def test_unknown_required_rejected(self):
        self.required(["missing.h"])
        with self.assertRaisesRegex(ValueError, "unknown"):
            self.check()

    def test_traversal_and_symlink_escape_rejected(self):
        for path in ("../escape", "/tmp/escape", "x/../escape"):
            with self.assertRaises(ValueError):
                verifier.safe_path(self.root, path)
        (self.root / "escape").symlink_to("/tmp")
        with self.assertRaises(ValueError):
            verifier.safe_path(self.root, "escape/file")

    def test_manifest_identity_corruption_rejected(self):
        path = self.root / "docs/IMPORT-MANIFEST.tsv"
        path.write_text(path.read_text().replace(verifier.PIN, "0" * 40))
        with self.assertRaisesRegex(ValueError, "identity"):
            self.check()

    def test_duplicate_required_rejected(self):
        self.required([self.name, self.name])
        with self.assertRaisesRegex(ValueError, "duplicate"):
            self.check()


if __name__ == "__main__":
    unittest.main()
