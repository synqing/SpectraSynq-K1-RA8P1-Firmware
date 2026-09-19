"""Pure HOST helpers only: never open CDC, launch a recorder or play audio."""
import importlib.util
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
spec = importlib.util.spec_from_file_location(
    'quiet_gap_runner', ROOT / 'scripts/run_quiet_gap.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class QuietGapRunner(unittest.TestCase):
    def test_room_rms_helper_completes_without_device_or_playback(self):
        with tempfile.TemporaryDirectory(prefix='k1-room-rms-') as temp:
            path = Path(temp) / 'host-only.wav'
            with wave.open(str(path), 'wb') as output:
                output.setnchannels(1)
                output.setsampwidth(2)
                output.setframerate(24000)
                output.writeframes(struct.pack('<4h', 1000, -1000, 1000, -1000))
            self.assertEqual(runner.wav_rms(path, 0, 1),
                             {'n': 4, 'rms': 1000.0, 'peak': 1000})

    def test_empty_truncated_or_oversized_frame_is_not_black(self):
        for size in (0, 1, 479, 481, 960):
            with self.subTest(size=size):
                with self.assertRaises(ValueError):
                    runner.frame_stats(bytes(size))

    def test_complete_zero_frame_is_black(self):
        self.assertEqual(runner.frame_stats(bytes(480))[:4], (0, 0, 0, 0))


if __name__ == '__main__':
    unittest.main()
