import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from test_gold_extract import compile_and_run, hop_pcm, parse, write_wav  # noqa: E402


class GoldExtractTests(unittest.TestCase):
    def test_host_extract_preserves_samples_and_wav_pcm(self):
        import struct
        import tempfile
        import wave

        with tempfile.TemporaryDirectory(prefix="k1-gold-extract-test-") as temp:
            directory = Path(temp)
            dump = directory / "packed.pcm"
            output = compile_and_run(directory / "test", dump)
            fields = parse(output.splitlines()[0])
            self.assertEqual(fields["K1_GOLD_EXTRACT"], "PASS")
            self.assertEqual(int(fields["samples_preserved"]), 8)
            self.assertGreaterEqual(int(fields["race_drop"]), 1)
            packed = dump.read_bytes()
            expected = b"".join(
                struct.pack("<" + "h" * 192, *hop_pcm(seq)) for seq in range(8)
            )
            self.assertEqual(packed, expected)
            wav = directory / "exact.wav"
            write_wav(wav, 8, packed)
            with wave.open(str(wav), "rb") as handle:
                self.assertEqual(handle.getnchannels(), 2)
                self.assertEqual(handle.getframerate(), 12800)
                self.assertEqual(handle.getsampwidth(), 2)
                self.assertEqual(handle.getnframes(), 96 * 8)
                decoded = handle.readframes(handle.getnframes())
            self.assertEqual(decoded, packed)


if __name__ == "__main__":
    unittest.main()
