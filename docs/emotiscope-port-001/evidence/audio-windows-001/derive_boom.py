#!/usr/bin/env python3
"""Derive/verify the approved BOOM development fixture; never plays audio."""

import argparse
import hashlib
import json
import math
from pathlib import Path
import platform
import subprocess
import sys
import wave

import numpy as np
import scipy
from scipy.io import wavfile


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
MEDIA = ROOT / "_scratch/waveform-migration-001/audio-boom-001"
SOURCE = Path("/Users/spectrasynq/Downloads/Tie\u0308sto-&-Sevenn-MrBlack-BOOM.mp3")
SOURCE_SHA256 = "3d60c58eccef198bb8062bef68df4d882c1b49c8f246623c4ba88d337cad8e4d"
OLD_ENTRIES_SHA256 = "6af1542fa1927dbd65ad58830fe007fb9dd6bac1f06df96301cb5c63e122887a"
FFMPEG = "/opt/homebrew/bin/ffmpeg"
FFPROBE = "/opt/homebrew/bin/ffprobe"
RATE, CHANNELS, FRAMES = 44100, 2, 3307500
HEADROOM_DB = 1.0
FLOAT = MEDIA / "native-first75s.wav"
PCM = MEDIA / "boom-0-75s-pcm16.wav"
METADATA = HERE / "boom-0-75s-derivation.json"
MANIFEST = HERE / "manifest.json"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def object_sha(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def run(argv):
    return subprocess.run(argv, check=True, capture_output=True)


def verify_wav(path, expected):
    with wave.open(str(path), "rb") as wav:
        require((wav.getframerate(), wav.getnchannels(), wav.getsampwidth(), wav.getnframes())
                == (RATE, CHANNELS, 2, FRAMES), "PCM16 WAV format/frame mismatch")
        samples = np.frombuffer(wav.readframes(FRAMES), dtype="<i2").reshape(FRAMES, CHANNELS)
    require(np.array_equal(samples, expected), "PCM differs from constant-gain quantisation")
    require(not np.any((samples == -32768) | (samples == 32767)), "PCM full-scale sample found")
    return samples


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify-only", action="store_true", help="Read/decode/verify; write nothing")
    args = parser.parse_args()
    require(sha(SOURCE) == SOURCE_SHA256, "Approved MP3 source hash mismatch")
    entries = json.loads(MANIFEST.read_text())
    require(object_sha(entries[:5]) == OLD_ENTRIES_SHA256, "Existing five manifest entries changed")
    require(len(entries) in (5, 6), "Unexpected manifest entries; reconcile ownership first")
    for entry in entries[:5]:
        require(sha(Path(entry["wav"])) == entry["wav_sha256"], "Existing WAV changed")

    probe_argv = [FFPROBE, "-v", "error", "-select_streams", "a:0", "-show_entries",
                  "format=start_time,duration:stream=index,codec_name,sample_rate,channels,channel_layout,start_time,time_base",
                  "-of", "json", str(SOURCE)]
    probe = json.loads(run(probe_argv).stdout)
    stream = probe["streams"][0]
    require((int(stream["sample_rate"]), stream["channels"]) == (RATE, CHANNELS),
            "Source is not native 44100 Hz stereo")
    decode_argv = [FFMPEG, "-v", "error", "-nostdin", "-c:a", "mp3float", "-i", str(SOURCE),
                   "-map", "0:a:0", "-af", f"atrim=start_sample=0:end_sample={FRAMES},asetpts=PTS-STARTPTS",
                   "-c:a", "pcm_f32le", "-f", "f32le", "pipe:1"]
    decoded = run(decode_argv)
    require(len(decoded.stdout) == FRAMES * CHANNELS * 4, "Decoded frame count mismatch")
    samples = np.frombuffer(decoded.stdout, dtype="<f4").reshape(FRAMES, CHANNELS)
    require(np.isfinite(samples).all(), "Nonfinite decoded samples")
    float_rate, retained = wavfile.read(FLOAT)
    require(float_rate == RATE and retained.dtype == np.float32 and np.array_equal(samples, retained),
            "Retained analytical float reference differs from fresh pinned-source decode")
    values = samples.astype(np.float64)
    peak = float(np.max(np.abs(values)))
    require(peak > 0, "Empty-energy source")
    gain = min(1.0, 10 ** (-HEADROOM_DB / 20) / peak)
    scaled = values * gain
    quantised = np.rint(scaled * 32768.0)
    require(np.min(quantised) > -32768 and np.max(quantised) < 32767,
            "Quantisation would reach full-scale; no clipping operation is permitted")
    pcm = quantised.astype("<i2")
    if not args.verify_only:
        # Keep the existing float reference and the five earlier WAVs untouched.
        with wave.open(str(PCM), "wb") as wav:
            wav.setnchannels(CHANNELS)
            wav.setsampwidth(2)
            wav.setframerate(RATE)
            wav.writeframes(pcm.tobytes())
    actual = verify_wav(PCM, pcm)
    error = actual.astype(np.float64) / 32768.0 - scaled
    maximum_error = float(np.max(np.abs(error)))
    require(maximum_error <= 0.5 / 32768.0 + 1e-15, "Quantisation residual exceeds half LSB")
    quantised_peak = float(np.max(np.abs(actual.astype(np.int32))) / 32768.0)
    metrics = {
        "frames": FRAMES, "scalar_samples": FRAMES * CHANNELS, "duration_s": FRAMES / RATE,
        "float_sample_peak_fs": peak, "float_samples_above_unity": int(np.count_nonzero(np.abs(values) > 1)),
        "constant_gain_linear": gain, "constant_gain_db": 20 * math.log10(gain),
        "scaled_sample_peak_fs": float(np.max(np.abs(scaled))), "declared_sample_headroom_db": HEADROOM_DB,
        "quantised_sample_peak_fs": quantised_peak, "quantised_sample_peak_dbfs": 20 * math.log10(quantised_peak),
        "pcm_min": int(actual.min()), "pcm_max": int(actual.max()), "pcm_full_scale_samples": 0,
        "max_quantisation_error_fs": maximum_error, "quantisation_error_limit_fs": 0.5 / 32768.0,
    }
    entry = {
        "track": "boom", "source": str(SOURCE), "source_sha256": SOURCE_SHA256,
        "wav": str(PCM), "wav_sha256": sha(PCM), "start_s": 0, "duration_s": 75,
        "rate": RATE, "channels": CHANNELS, "sample_format": "pcm_s16le", "frames": FRAMES,
        "normalisation": "one constant attenuation from complete 0..75s float sample peak; no section normalisation",
        "gain_linear": gain, "gain_db": metrics["constant_gain_db"], "sample_headroom_db": HEADROOM_DB,
        "cue_domain": "first decoded sample of pinned MP3; 0..75 s; not MP3 container timestamps",
        "suite_role": "development", "lineage_id": "boom:" + SOURCE_SHA256,
        "lineage_policy": "All BOOM derivatives are one development song lineage, never independent holdouts",
        "scored_window_s": [28, 75],
        "descriptors": [
            {"window_s": [31, 42], "description": "build with increasing event density", "status": "tentative descriptive region; no exact onset truth"},
            {"window_s": [46, 50], "description": "pre-cut build", "status": "tentative descriptive region; no exact onset truth"},
            {"window_s": [50, 55], "description": "cut and reduced-level interval", "status": "descriptive region, not exact silence"},
            {"window_s": [55, 55.5], "description": "re-entry transition", "status": "tentative subdivisions; triplet interpretation unconfirmed"},
            {"window_s": [55.5, 75], "description": "post-entry rhythmic pattern", "status": "tentative subdivisions; no hardcoded BPM truth"},
        ],
        "derivation_metadata": str(METADATA),
    }
    if args.verify_only:
        metadata = json.loads(METADATA.read_text())
        require(metadata["metrics"] == metrics, "Recorded metrics differ")
        require(metadata["reproduction"]["code_sha256"] == sha(Path(__file__)), "Derivation script identity changed")
        require(metadata["inputs"]["float_reference_sha256"] == sha(FLOAT), "Float reference hash changed")
        entry["derivation_metadata_sha256"] = sha(METADATA)
        require(len(entries) == 6 and entries[5] == entry, "BOOM manifest entry differs from verified data")
    else:
        metadata = {
            "schema_version": "scipy-evidence/1.0", "analysis_status": "complete",
            "evidence_kind": "offline source-bound audio fixture derivation", "primary_domain": "audio-dsp",
            "domains_used": ["audio-dsp"], "question": "Produce the approved 0..75s BOOM PCM16 development fixture without clipping or time-varying gain",
            "inputs": {"source": str(SOURCE), "source_sha256": SOURCE_SHA256,
                       "float_reference": str(FLOAT), "float_reference_sha256": sha(FLOAT),
                       "decoded_f32le_sha256": hashlib.sha256(decoded.stdout).hexdigest(),
                       "axes": ["frame", "stereo channel"], "dtype": "IEEE754 float32 little-endian",
                       "rate_hz": RATE, "channels": CHANNELS, "frames": FRAMES,
                       "exclusions": "source frames after 3307499; no exclusions within 0..75s"},
            "authority": {"task": "Captain-approved CanonicalAudioPerformanceSuite v0.1 WP2", "missing_definitions": [],
                          "charter": "SPECTRASYNQ-PRODUCT-DEVELOPMENT-01 v1.0.0",
                          "charter_sha256": "5c2d54ad5189892c0cd633afa60cffcafb2880c4f74c3916ca6b782f225da74e"},
            "environment": {"interpreter": sys.executable, "python": sys.version, "numpy": np.__version__,
                            "scipy": scipy.__version__, "platform": platform.platform(),
                            "ffmpeg_version": run([FFMPEG, "-version"]).stdout.decode().splitlines()[0],
                            "decoder": "FFmpeg mp3float", "ffmpeg_sha256": sha(Path(FFMPEG))},
            "method": {"first_sample_origin": "t=0 is first decoded sample yielded by FFmpeg after normal MP3 decoder skip handling; no input seek",
                       "mp3_container_start_time_s": probe["format"].get("start_time"),
                       "mp3_audio_stream_start_time_s": stream.get("start_time"), "source_probe": probe,
                       "sample_time_s": "frame_index / 44100, indices 0..3307499; interval [0,75)",
                       "gain_formula": "min(1, 10**(-1/20) / max(abs(complete derived float samples)))",
                       "gain_scope": "same scalar for every frame and both channels",
                       "quantisation": "numpy.rint(scaled_float64 * 32768), nearest ties-to-even, then signed16; no dither or clipping",
                       "downmix": False, "resample": False, "per_section_normalisation": False,
                       "peak_definition": "maximum absolute decoded sample, not true peak or acoustic clipping"},
            "metrics": metrics,
            "diagnostics": {"fresh_decode_equals_retained_float": True, "nonfinite_samples": 0,
                            "constant_gain_pcm_equality": True, "old_entry_count": 5,
                            "old_entries_sha256": OLD_ENTRIES_SHA256, "old_wav_hashes_unchanged": True,
                            "decoder_stderr": decoded.stderr.decode()},
            "gate": {"status": "pass", "criterion": "pinned source, exact frames/native stereo rate, constant gain, half-LSB quantisation residual, no PCM full-scale samples, existing entries/WAVs preserved",
                     "authority": "WP2 parent assignment", "reason": "Direct decoded array and WAV readback checks passed; fixture scope only"},
            "reproduction": {"argv": [sys.executable, str(Path(__file__).resolve())],
                             "verification_argv": [sys.executable, str(Path(__file__).resolve()), "--verify-only"],
                             "code_sha256": sha(Path(__file__)), "probe_argv": probe_argv, "decode_argv": decode_argv,
                             "outputs": {"wav": str(PCM), "wav_sha256": sha(PCM), "manifest": str(MANIFEST)}},
            "limitations": ["Sample headroom does not establish inter-sample true peak, DAC, loudspeaker or microphone clipping.",
                            "No playback, target, acoustic, LED or optical validation performed.",
                            "Rhythm descriptors are tentative; no exact BPM or onset annotation truth is asserted.",
                            "All BOOM derivatives remain one development lineage. Dopex is untouched."],
        }
        METADATA.write_text(json.dumps(metadata, indent=2, allow_nan=False) + "\n")
        entry["derivation_metadata_sha256"] = sha(METADATA)
        MANIFEST.write_text(json.dumps(entries[:5] + [entry], indent=2, allow_nan=False) + "\n")
    print(json.dumps({"verification": "pass", "wav": str(PCM), "wav_sha256": sha(PCM), "metrics": metrics,
                      "original_entries_unchanged": True, "mode": "verify-only" if args.verify_only else "derive"}, indent=2, allow_nan=False))


if __name__ == "__main__":
    main()
