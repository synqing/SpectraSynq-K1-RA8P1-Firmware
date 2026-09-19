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
    predict_from_witness,
    score_capture,
    score_path,
    self_test,
    DMA_WORDS,
    WIRE_BITS,
)


class ScoreP601Capture(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.predicted = predict_from_witness()

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
        with self.assertRaises(CaptureScoreError) as raised:
            score_path(historical, self.predicted)
        self.assertIn(raised.exception.code,
                      ('K1_P601_IDENTITY', 'K1_P601_FRAME', 'K1_P601_PROBE_MISSING'))

    def test_short_and_synthetic_never_stamp_waveform_captured(self):
        short = complete_synthetic(self.predicted, bit_count=WIRE_BITS - 1)
        scored = score_capture(short, self.predicted)
        self.assertEqual(scored['waveform'], 'NOT_CAPTURED')
        self.assertFalse(scored['bit_count_ok'])
        complete = complete_synthetic(self.predicted)
        scored = score_capture(complete, self.predicted)
        self.assertEqual(scored['waveform'], 'NOT_CAPTURED')
        self.assertTrue(scored['bit_count_ok'])
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

    def test_self_test_bundle_passes_without_claiming_wire(self):
        result = self_test()
        self.assertTrue(result['pass'])
        self.assertEqual(result['waveform'], 'NOT_CAPTURED')
        self.assertEqual(result['K1_P601_SELFTEST'], 'PASS')

    def test_written_capture_file_scores(self):
        with tempfile.TemporaryDirectory(prefix='k1-p601-') as temp:
            path = Path(temp) / 'capture.json'
            path.write_text(json.dumps(
                complete_synthetic(self.predicted, source='physical')))
            scored = score_path(path, self.predicted)
            self.assertEqual(scored['waveform'], 'WAVEFORM_CAPTURED')
            self.assertTrue(scored['bit_count_ok'])


if __name__ == '__main__':
    unittest.main()
