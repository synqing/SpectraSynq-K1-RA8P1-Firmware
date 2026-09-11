#!/usr/bin/env python3
"""HOST gold-extract: compile, run, exact WAV PCM round-trip, backend cost."""
from __future__ import annotations

import os
import struct
import tempfile
import wave
from pathlib import Path

from run_host import run
from verify_imports import ROOT

PLATFORM = ROOT / "platform/ra8p1"
SOURCES = [
    PLATFORM / "titan_ram_diag.c",
    PLATFORM / "k1_exact_stream.c",
    PLATFORM / "ws281x_waveform.c",
    PLATFORM / "k1_shared_snapshot.c",
]


def compile_and_run(binary: Path, dump: Path | None = None) -> str:
    command = [
        "c++",
        "-std=c++17",
        "-O2",
        "-ffp-contract=off",
        "-fno-fast-math",
        "-I" + str(ROOT / "src/k1"),
        "-I" + str(PLATFORM),
        str(ROOT / "tests/host/test_gold_extract.cpp"),
        *[str(path) for path in SOURCES],
        "-o",
        str(binary),
    ]
    run(command)
    env = os.environ.copy()
    if dump is not None:
        env["K1_EXACT_DUMP"] = str(dump)
    result = __import__("subprocess").run(
        [str(binary)], capture_output=True, text=True, timeout=180, env=env
    )
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def hop_pcm(seq: int) -> list[int]:
    pcm = []
    for i in range(96 * 2):
        value = (int(seq * 17 + i * 3) - 4000) & 0xFFFFFFFF
        if value >= 0x80000000:
            value -= 0x100000000
        packed = struct.pack("<i", value)[:2]
        pcm.append(struct.unpack("<h", packed)[0])
    pcm[0] = 32767
    pcm[1] = -32768
    pcm[2] = -1
    return pcm


def write_wav(path: Path, hops: int, pcm_bytes: bytes | None = None) -> None:
    if pcm_bytes is None:
        frames: list[int] = []
        for seq in range(hops):
            frames.extend(hop_pcm(seq))
        pcm_bytes = struct.pack("<" + "h" * len(frames), *frames)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(2)
        handle.setsampwidth(2)
        handle.setframerate(12800)
        handle.writeframes(pcm_bytes)


def parse(line: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for token in line.split():
        if "=" in token:
            key, value = token.split("=", 1)
            out[key] = value
    return out


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="k1-gold-extract-") as temp:
        directory = Path(temp)
        dump = directory / "packed.pcm"
        output = compile_and_run(directory / "test", dump)
        print(output, end="")
        fields = parse(output.splitlines()[0])
        assert fields["K1_GOLD_EXTRACT"] == "PASS"
        assert int(fields["exact_hops"]) == 9
        assert int(fields["discontinuities"]) == 1
        assert int(fields["samples_preserved"]) == 8
        assert int(fields["gpt80_data_bytes"]) == 15360
        assert int(fields["gpt80_data_ns"]) == 4800000
        assert int(fields["gpt80_reset_ns"]) == 300000
        assert int(fields["podr80_bytes"]) == 204000
        assert int(fields["gpt160_data_bytes"]) == 30720
        assert int(fields["gpt160_emit_ns"]) == 9900000
        assert int(fields["podr160_bytes"]) == 396000
        assert int(fields["double_buf_2x160"]) == 122880
        assert int(fields["race_drop"]) >= 1
        packed = dump.read_bytes()
        expected = b"".join(
            struct.pack("<" + "h" * 192, *hop_pcm(seq)) for seq in range(8)
        )
        assert packed == expected
        wav = directory / "exact_12800_96.wav"
        write_wav(wav, 8, packed)
        with wave.open(str(wav), "rb") as handle:
            assert handle.getnchannels() == 2
            assert handle.getframerate() == 12800
            assert handle.getsampwidth() == 2
            assert handle.getnframes() == 96 * 8
            decoded = handle.readframes(handle.getnframes())
        assert decoded == packed
        samples = struct.unpack("<" + "h" * (len(decoded) // 2), decoded)
        assert -32768 in samples and 32767 in samples and -1 in samples
        print(
            "K1_LED_BACKEND_COMPARE=PASS gpt80_data_bytes=%s podr80_bytes=%s "
            "gpt160_data_bytes=%s wav_pcm_exact=PASS"
            % (
                fields["gpt80_data_bytes"],
                fields["podr80_bytes"],
                fields["gpt160_data_bytes"],
            )
        )


if __name__ == "__main__":
    main()
