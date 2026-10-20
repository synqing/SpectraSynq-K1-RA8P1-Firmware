import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from tempo_dtcm_overlay import (
    apply_tempo_placement,
    verify_tempo_elf_placement,
    verify_tempo_placement_receipt,
)


DTCM_SYMBOLS = (
    '20000000 b k1::core::audio::(anonymous namespace)::'
    'computeTempoAcfAtRate(TempoNoveltyRing const&, unsigned short, float, float, TempoAcfOutput&)::work\n'
    '20001000 b k1::core::audio::(anonymous namespace)::'
    'computeTempoAcfAtRate(TempoNoveltyRing const&, unsigned short, float, float, TempoAcfOutput&)::acf\n'
)
SRAM_SYMBOLS = DTCM_SYMBOLS.replace('20000000', '22010000').replace('20001000', '22011000')
ITCM_SYMBOLS = DTCM_SYMBOLS.replace('20000000', '00001000').replace('20001000', '00002000')


class TempoPlacementTests(unittest.TestCase):
    def test_receipt_rejects_mismatched_overlay(self):
        verify_tempo_placement_receipt({'tempo_placement': 'empty-tcm'})
        verify_tempo_placement_receipt({
            'tempo_placement': 'dtcm',
            'tempo_dtcm_sources': {'core/audio/tempo_acf.cpp': {}},
        })
        with self.assertRaisesRegex(RuntimeError, 'DTCM overlay'):
            verify_tempo_placement_receipt({
                'tempo_placement': 'empty-tcm',
                'tempo_dtcm_sources': {'core/audio/tempo_acf.cpp': {}},
            })
        with self.assertRaisesRegex(RuntimeError, 'missing'):
            verify_tempo_placement_receipt({'tempo_placement': 'dtcm'})

    def test_elf_empty_tcm_rejects_tcm_symbols(self):
        evidence = verify_tempo_elf_placement('', 'empty-tcm')
        self.assertEqual(evidence['residence'], 'automatic-storage')
        self.assertEqual(evidence['stack_delta_bytes'], 2848)
        with self.assertRaisesRegex(RuntimeError, 'TCM'):
            verify_tempo_elf_placement(DTCM_SYMBOLS, 'empty-tcm')
        with self.assertRaisesRegex(RuntimeError, 'static'):
            verify_tempo_elf_placement(SRAM_SYMBOLS, 'empty-tcm')

    def test_elf_dtcm_requires_dtcm_addresses(self):
        evidence = verify_tempo_elf_placement(DTCM_SYMBOLS, 'dtcm')
        self.assertEqual(evidence['residence'], 'section .dtcm')
        with self.assertRaisesRegex(RuntimeError, 'missing'):
            verify_tempo_elf_placement('', 'dtcm')
        with self.assertRaisesRegex(RuntimeError, 'DTCM'):
            verify_tempo_elf_placement(SRAM_SYMBOLS, 'dtcm')
        with self.assertRaisesRegex(RuntimeError, 'ITCM'):
            verify_tempo_elf_placement(ITCM_SYMBOLS, 'dtcm')

    def test_staged_empty_tcm_does_not_rewrite(self):
        with tempfile.TemporaryDirectory() as temporary:
            staged = Path(temporary) / 'k1' / 'core' / 'audio'
            staged.mkdir(parents=True)
            source = ROOT / 'src/k1/core/audio/tempo_acf.cpp'
            target = staged / 'tempo_acf.cpp'
            target.write_bytes(source.read_bytes())
            record = apply_tempo_placement(staged.parents[1], 'empty-tcm')
            self.assertEqual(record['placement'], 'empty-tcm')
            self.assertEqual(target.read_bytes(), source.read_bytes())
            apply_tempo_placement(staged.parents[1], 'dtcm')
            self.assertIn('section(".dtcm")', target.read_text())

    def test_build_scalar_requires_named_placement(self):
        missing = subprocess.run(
            [sys.executable, str(ROOT / 'scripts/build_scalar.py'),
             '--output', '/tmp/k1-tempo-placement-must-not-exist',
             '--resident-controls', '/dev/null'],
            capture_output=True, text=True)
        self.assertNotEqual(missing.returncode, 0)
        self.assertIn('--tempo-placement', missing.stderr)
        self.assertFalse(Path('/tmp/k1-tempo-placement-must-not-exist').exists())


if __name__ == '__main__':
    unittest.main()
