"""Bounded ownership and linked-placement fault checks; no target I/O."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'scripts'))
from tempo_dtcm_overlay import validate_tempo_placement_owner, verify_tempo_elf_placement


class TempoWorkspace(unittest.TestCase):
    def test_owner(self):
        validate_tempo_placement_owner(None, False, True, True, True)
        validate_tempo_placement_owner('dtcm', False, True, True, True)
        validate_tempo_placement_owner('empty-tcm', True, False, False, False)
        for donor, tempo, live in [(False, True, True), (True, False, True), (True, True, False)]:
            with self.assertRaises(ValueError):
                validate_tempo_placement_owner('dtcm', False, donor, tempo, live)
        with self.assertRaises(ValueError):
            validate_tempo_placement_owner(None, True, False, False, False)

    def test_exact_zero_workspace(self):
        symbols = ('20000000 b k1::core::audio::computeTempoAcfAtRate(...)::work\n'
                   '20000800 b k1::core::audio::computeTempoAcfAtRate(...)::acf\n'
                   '20000000 ? __dtcm_zero$$Base\n20000b20 ? __dtcm_zero$$Limit\n'
                   '20000000 ? __dtcm_noinit$$Base\n20000000 ? __dtcm_noinit$$Limit\n')
        result = verify_tempo_elf_placement(symbols, 'dtcm', scratch_only=True)
        self.assertEqual(result['zero_initialised_bytes'], 2848)
        faults = [symbols.replace('20000b20', '20000b40'),
                  symbols.replace('20000800 b', '20000820 b'),
                  symbols.replace('20000000 ? __dtcm_noinit$$Limit', '20000020 ? __dtcm_noinit$$Limit'),
                  symbols.replace('20000000 b', '22000000 b'),
                  symbols + '20000400 b k1::core::audio::computeTempoAcfAtRate(...)::work\n']
        for fault in faults:
            with self.assertRaises(RuntimeError):
                verify_tempo_elf_placement(fault, 'dtcm', scratch_only=True)


if __name__ == '__main__':
    unittest.main()
