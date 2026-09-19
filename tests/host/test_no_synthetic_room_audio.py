"""HARD FAIL: never play white noise or computer-generated tones in the room."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2] / 'scripts'
PLAY = re.compile(r'\b(afplay|ffplay|paplay|aplay)\b')
SYNTH = re.compile(
    r'tone_wav|write_tone|hit_wav|click_wav|sparse_clicks|pad_chord|'
    r'percussion\(|white.?noise|math\.sin\(2 \* math\.pi',
    re.I)


class NoSyntheticRoomAudio(unittest.TestCase):
    def test_scripts_do_not_play_generated_tones_or_noise(self):
        bad = []
        for path in sorted(ROOT.glob('*.py')):
            text = path.read_text()
            if PLAY.search(text) and SYNTH.search(text):
                bad.append(path.name)
        self.assertEqual(
            bad, [],
            'scripts play computer-generated tones or noise: ' + ', '.join(bad))


if __name__ == '__main__':
    unittest.main()
