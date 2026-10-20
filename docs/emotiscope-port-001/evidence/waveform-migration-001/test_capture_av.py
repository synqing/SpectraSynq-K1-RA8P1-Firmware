"""Offline regressions for the specific missing-audio and truncated-passage bugs."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import wave

import numpy as np

import capture_existing as capture
from capture_av_validation import (validate_fixture, require_reserve,
                                   validate_player_completion, match_source, validate_recording)


class CaptureContract(unittest.TestCase):
    def test_retained_24_second_fixtures_and_format_mismatch(self):
        rows = json.loads((capture.F / 'docs/emotiscope-port-001/evidence/audio-windows-001/manifest.json').read_text())
        for row in rows:
            self.assertEqual(validate_fixture(row)['duration_s'], row['duration_s'])
        row = rows[0]
        for change in ({'wav_sha256': '0' * 64}, {'channels': 2}, {'rate': 44100},
                       {'frames': 12}, {'sample_format': 'pcm_f32le'}):
            with self.assertRaises(RuntimeError):
                validate_fixture({**row, **change})

    def test_75_second_format_and_no_early_completion(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / '75s.wav'
            with wave.open(str(path), 'wb') as stream:
                stream.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))
                stream.writeframes(bytes(75 * 44100 * 4))
            record = dict(wav=str(path), wav_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                          duration_s=75, rate=44100, channels=2, frames=3307500)
            self.assertEqual(validate_fixture(record)['frame_count'], 3307500)
        validate_player_completion(0, 10, 85, 75)
        for rc, end in ((0, 34), (0, 84), (1, 85)):
            with self.assertRaises(RuntimeError):
                validate_player_completion(rc, 10, end, 75)
        require_reserve(120, 8, 91 + 4)
        with self.assertRaises(RuntimeError):
            require_reserve(100, 13, 89)

    def test_missing_audio_track_rejected_before_decode(self):
        result = type('Result', (), {'stdout': json.dumps({'streams': [{'codec_type': 'video'}]})})()
        with patch('capture_av_validation.subprocess.run', return_value=result):
            with self.assertRaisesRegex(RuntimeError, 'microphone track'):
                validate_recording('recording.mov', 'source.wav', 2, 0, 24)

    def test_external_camera_requires_explicit_matching_microphone(self):
        result = type('Result', (), {'stdout': f'{capture.CAMERA} capture {capture.CAMERA_UID} 100 /tmp/a.mov operator-product-frame-confirmed 9999999999'})()
        with patch('capture_existing.subprocess.run', return_value=result):
            with self.assertRaisesRegex(RuntimeError, 'microphone argument'):
                capture.external_camera(123, Path('/not-read'), 89, 'requested-mic')

    def test_three_landmarks_match_received_source_but_reject_room_noise(self):
        # Distinct musical-envelope-like landmarks, delayed acoustic path and
        # gain/noise. No playback or capture API is invoked by this test.
        rng = np.random.default_rng(731)
        rate, seconds = 8000, 24
        t = np.arange(rate * seconds) / rate
        envelope = np.repeat(rng.uniform(.02, 1, seconds * 20), rate // 20)
        source = envelope * (np.sin(2 * np.pi * 173 * t) + .4 * np.sin(2 * np.pi * 687 * t)
                             + .2 * np.sin(2 * np.pi * 1711 * t))
        received = np.concatenate([np.zeros(2 * rate), source * .17, np.zeros(3 * rate)])
        received += rng.normal(0, .0005, len(received))
        result = match_source(received, source, 2, 0, seconds)
        self.assertTrue(result['passed'], result)
        self.assertAlmostEqual(result['source_to_audio_offset_s'], 2, delta=.03)
        unrelated = rng.normal(0, .01, len(received))
        self.assertFalse(match_source(unrelated, source, 2, 0, seconds)['passed'])
        with self.assertRaisesRegex(RuntimeError, 'insufficient'):
            match_source(received[:rate * 15], source, 2, 0, seconds)


if __name__ == '__main__':
    unittest.main()
