#!/usr/bin/env python3
"""Fail-closed P601/DIN scorer: missing capture and overclaim never pass."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from score_p601_capture import (
    CaptureScoreError,
    complete_synthetic,
    physical_with_files,
    retained_zero_frame_prediction,
    score_capture,
    score_path,
    self_test,
    DMA_WORDS,
    WIRE_BITS,
)


class ScoreP601Capture(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.predicted = retained_zero_frame_prediction()

    def test_predicted_reference_matches_retained_zero_frame(self):
        predicted = self.predicted
        self.assertEqual(predicted['bits'], WIRE_BITS)
        self.assertEqual(predicted['expected_dma_words'], DMA_WORDS)
        self.assertEqual(predicted['pixels'], 128)
        self.assertTrue(predicted['all_zero_grb'])
        self.assertEqual(predicted['first_bit'], 0)
        self.assertEqual(predicted['last_bit'], 0)
        self.assertEqual(predicted['first_high_ns'], 250)
        self.assertEqual(predicted['t0h_duty_counts'], 74)
        self.assertEqual(predicted['waveform'], 'NOT_CAPTURED')
        self.assertFalse(predicted['physical_claims'])
        self.assertEqual(
            predicted['packed_grb_sha256'],
            'a1a4f5721c1c4610af7f71078f3a68c330536d679803b0e0507ee8dc10c5dfca')

    def test_missing_capture_fails(self):
        with self.assertRaisesRegex(CaptureScoreError, 'capture missing'):
            score_path(Path('/tmp/k1-p601-does-not-exist.json'), self.predicted)

    def test_historical_ws2812_diagnostic_is_not_the_retained_frame(self):
        historical = Path(
            '/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/'
            'k1-ra8p1-002/ws2812-p601-live-01/receipt.json')
        if not historical.is_file():
            self.skipTest('optional Mac historical receipt not present')
        with self.assertRaises(CaptureScoreError) as raised:
            score_path(historical, self.predicted)
        self.assertIn(raised.exception.code,
                      ('K1_P601_IDENTITY', 'K1_P601_FRAME', 'K1_P601_PROBE_MISSING'))

    def test_short_and_synthetic_never_stamp_waveform_captured(self):
        short = complete_synthetic(self.predicted, bit_count=WIRE_BITS - 1)
        scored = score_capture(short, self.predicted)
        self.assertEqual(scored['waveform'], 'NOT_CAPTURED')
        self.assertFalse(scored['bit_count_ok'])
        self.assertFalse(scored['pass'])
        complete = complete_synthetic(self.predicted)
        scored = score_capture(complete, self.predicted)
        self.assertEqual(scored['waveform'], 'NOT_CAPTURED')
        self.assertTrue(scored['bit_count_ok'])
        self.assertFalse(scored['pass'])
        with self.assertRaisesRegex(CaptureScoreError, 'physical'):
            score_capture(dict(complete, waveform='WAVEFORM_CAPTURED'),
                          self.predicted)

    def test_empty_object_and_placeholder_hash_fail(self):
        with self.assertRaises(CaptureScoreError):
            score_capture({}, self.predicted)
        capture = complete_synthetic(self.predicted, source='physical')
        capture['p601']['capture_sha256'] = '0' * 64
        with self.assertRaisesRegex(CaptureScoreError, 'placeholder'):
            score_capture(capture, self.predicted)

    def test_physical_json_without_raw_files_is_not_qualification(self):
        capture = complete_synthetic(self.predicted, source='physical')
        capture['p601']['first_bit_high_ns'] = -10
        capture['din']['first_bit_high_ns'] = -10
        capture['p601']['last_bit_high_ns'] = -10
        capture['din']['last_bit_high_ns'] = -10
        capture['p601']['reset_low_ns'] = 0
        capture['din']['reset_low_ns'] = 0
        capture['p601']['capture_sha256'] = 'z' * 64
        capture['din']['capture_sha256'] = 'z' * 64
        scored = score_capture(capture, self.predicted)
        self.assertFalse(scored['pass'])
        self.assertEqual(scored['waveform'], 'NOT_CAPTURED')
        self.assertFalse(scored['physical_claims'])
        self.assertFalse(scored['raw_files_verified'])
        self.assertIsNotNone(scored['first_divergence'])

    def test_written_capture_file_without_raw_files_is_not_waveform(self):
        with tempfile.TemporaryDirectory(prefix='k1-p601-') as temp:
            path = Path(temp) / 'capture.json'
            path.write_text(json.dumps(
                complete_synthetic(self.predicted, source='physical')))
            scored = score_path(path, self.predicted)
            self.assertEqual(scored['waveform'], 'NOT_CAPTURED')
            self.assertFalse(scored['pass'])
            self.assertFalse(scored['physical_claims'])

    def test_hashed_raw_files_with_conforming_pulses_qualify(self):
        with tempfile.TemporaryDirectory(prefix='k1-p601-raw-') as temp:
            capture = physical_with_files(self.predicted, temp)
            scored = score_capture(capture, self.predicted)
            self.assertTrue(scored['raw_files_verified'])
            self.assertTrue(scored['frame_correlated'])
            self.assertTrue(scored['waveform_conforms'])
            self.assertTrue(scored['pass'])
            self.assertEqual(scored['waveform'], 'WAVEFORM_CAPTURED')
            self.assertTrue(scored['physical_claims'])
            self.assertIsNone(scored['first_divergence'])

    def test_hashed_raw_files_with_illegal_pulses_remain_failing_evidence(self):
        with tempfile.TemporaryDirectory(prefix='k1-p601-bad-') as temp:
            capture = physical_with_files(
                self.predicted, temp, first_high=-10, last_high=-10, reset_low=0)
            scored = score_capture(capture, self.predicted)
            self.assertTrue(scored['capture_present'])
            self.assertTrue(scored['raw_files_verified'])
            self.assertTrue(scored['physical_claims'])
            self.assertFalse(scored['pass'])
            self.assertEqual(scored['waveform'], 'NOT_CAPTURED')
            self.assertFalse(scored['waveform_conforms'])
            self.assertIn(scored['first_divergence'],
                          ('negative_timing', 'reset_low', 'first_high', 'last_high'))

    def test_self_test_bundle_passes_without_claiming_wire(self):
        result = self_test()
        self.assertTrue(result['pass'])
        self.assertEqual(result['waveform'], 'NOT_CAPTURED')
        self.assertEqual(result['K1_P601_SELFTEST'], 'PASS')
        self.assertIn('PHYSICAL_WITHOUT_FILES_REJECTED', result['proofs'])


if __name__ == '__main__':
    unittest.main()
