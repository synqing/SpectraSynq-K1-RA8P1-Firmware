"""Offline validity checks for the existing Titan camera capture, never playback.

Source correspondence uses microphone band-energy landmarks, not light peaks.
The reported map is an acoustic proxy, not MCU PCM or microphone-to-photon delay.
"""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import subprocess
import wave


def validate_fixture(record):
    path = Path(record['wav'])
    if hashlib.sha256(path.read_bytes()).hexdigest() != record['wav_sha256']:
        raise RuntimeError('approved audio window hash mismatch')
    with wave.open(str(path)) as stream:
        actual = dict(frame_count=stream.getnframes(), rate=stream.getframerate(),
                      channels=stream.getnchannels(), sample_width_bytes=stream.getsampwidth(),
                      compression=stream.getcomptype())
    # The original manifest predates format fields: its retained fixtures were
    # explicitly mono s16le. New entries declare their actual channel count.
    if record.get('sample_format', 'pcm_s16le') != 'pcm_s16le':
        raise RuntimeError('approved fixture must declare PCM signed 16-bit format')
    expected = dict(rate=record['rate'], channels=record.get('channels', 1),
                    sample_width_bytes=record.get('sample_width_bytes', 2), compression='NONE',
                    frame_count=record.get('frames', record.get('frame_count', round(record['duration_s'] * record['rate']))))
    if actual != expected:
        raise RuntimeError(f'approved audio format/frame count mismatch: {actual} != {expected}')
    duration = actual['frame_count'] / actual['rate']
    if not math.isfinite(duration) or duration <= 0 or abs(duration - record['duration_s']) > 1 / actual['rate']:
        raise RuntimeError('approved audio duration mismatch')
    return {**actual, 'duration_s': duration}


def require_reserve(deadline, now, needed):
    if not all(math.isfinite(v) for v in (deadline, now, needed)) or needed < 0 or deadline - now < needed:
        raise RuntimeError('camera lacks time for complete playback, post-roll and finalisation reserve')


def validate_player_completion(returncode, started, ended, expected_seconds):
    # A small process scheduling tolerance does not allow a truncated passage;
    # received-audio correspondence and coverage are checked independently.
    if returncode != 0 or ended - started < expected_seconds - 0.15:
        raise RuntimeError(f'approved music playback ended early or failed: {ended - started:.3f}s / {expected_seconds:.3f}s, rc={returncode}')


def decode_audio(path, ffmpeg='ffmpeg'):
    import numpy as np
    result = subprocess.run([ffmpeg, '-v', 'error', '-i', str(path), '-map', '0:a:0',
                             '-ac', '1', '-ar', '8000', '-f', 'f32le', 'pipe:1'],
                            capture_output=True, timeout=30, check=True)
    samples = np.frombuffer(result.stdout, dtype='<f4')
    if not len(samples) or not np.isfinite(samples).all():
        raise RuntimeError('captured microphone has no finite decoded samples')
    return samples


def energy_features(samples):
    import numpy as np
    from scipy.signal import butter, sosfilt
    # 20 ms analysis bins are a correspondence resolution, not onset accuracy.
    hop = 160
    size = len(samples) // hop
    rows = []
    for low, high in ((80, 250), (250, 1000), (1000, 3000)):
        signal = sosfilt(butter(3, [low, high], fs=8000, btype='band', output='sos'), samples)
        power = np.mean(signal[:size * hop].reshape(size, hop) ** 2, axis=1)
        rows.append(np.log(np.maximum(power, 1e-12)))
    return np.asarray(rows)


def match_source(received, reference, expected_offset, score_start, score_end):
    """Three separated landmarks; reject noise, wrong passages and ambiguous ties.

    The fixed v1 thresholds are deliberately conservative capture checks. A
    failure is capture-invalid/inconclusive, not a firmware failure or licence
    to tune the thresholds until an unknown recording passes.
    """
    import numpy as np
    from scipy.signal import correlate
    mic, source = energy_features(received), energy_features(reference)
    hop_s, width_s = .02, 4.0
    width = round(width_s / hop_s)
    duration = len(reference) / 8000
    if duration < 12 or not 0 <= score_start < score_end <= duration + .001:
        raise RuntimeError('invalid source/scored interval for correspondence')
    centres = np.linspace(max(2.5, score_start + 2), min(duration - 2.5, score_end - 2), 3)
    matches = []
    for centre in centres:
        start = round((centre - width_s / 2) / hop_s)
        ref = source[:, start:start + width]
        ref = ref - ref.mean(axis=1, keepdims=True)
        ref_norm = np.sqrt(np.sum(ref ** 2, axis=1))
        # Offset derives from host process/camera start only to bound the search.
        # The final measured map comes from received musical landmarks.
        low = max(0, math.floor((start * hop_s + expected_offset - 3) / hop_s))
        high = min(mic.shape[1], math.ceil((start * hop_s + expected_offset + 3 + width_s) / hop_s))
        target = mic[:, low:high]
        if target.shape[1] < width or np.any(ref_norm < 1e-6):
            raise RuntimeError('insufficient or featureless audio for source landmark')
        scores = []
        kernel = np.ones(width)
        for band in range(3):
            sums = np.convolve(target[band], kernel, 'valid')
            squares = np.convolve(target[band] ** 2, kernel, 'valid')
            norm = np.sqrt(np.maximum(squares - sums ** 2 / width, 0)) * ref_norm[band]
            scores.append(correlate(target[band], ref[band], mode='valid') / np.maximum(norm, 1e-12))
        scores = np.mean(scores, axis=0)
        peak = int(np.argmax(scores))
        # Neighbouring bins describe one broad acoustic peak, not competing IDs.
        alternate = scores.copy()
        alternate[max(0, peak - 15):peak + 16] = -1
        margin = float(scores[peak] - alternate.max())
        matches.append(dict(source_s=start * hop_s, received_s=(low + peak) * hop_s,
                            correlation=float(scores[peak]), distinct_peak_margin=margin))
    x = np.array([r['source_s'] for r in matches])
    y = np.array([r['received_s'] for r in matches])
    slope, offset = np.polyfit(x, y, 1)
    residual = float(np.max(np.abs(y - (offset + slope * x))))
    passed = all(r['correlation'] >= .45 and r['distinct_peak_margin'] >= .03 for r in matches)
    passed = passed and abs(slope - 1) <= .005 and residual <= .12
    return dict(passed=bool(passed), method='three-band log energy NCC v1',
                analysis_bin_s=hop_s, landmark_width_s=width_s, landmarks=matches,
                minimum_correlation=.45, minimum_distinct_peak_margin=.03,
                maximum_absolute_drift=.005, maximum_residual_s=.12,
                source_to_audio_offset_s=float(offset), source_to_audio_scale=float(slope),
                max_landmark_residual_s=residual,
                map_uncertainty_s=hop_s + residual,
                timing_scope='received microphone acoustic proxy; not MCU input or photon latency')


def validate_recording(path, source, expected_offset, score_start, score_end):
    probe = subprocess.run(['ffprobe', '-v', 'error', '-show_streams', '-show_format',
                            '-show_frames', '-show_entries',
                            'stream=index,codec_type,codec_name,sample_rate,channels,start_time,duration:format=duration:frame=media_type,best_effort_timestamp_time,nb_samples,pkt_duration_time',
                            '-of', 'json', str(path)], capture_output=True, text=True,
                           timeout=30, check=True)
    data = json.loads(probe.stdout)
    audio = [s for s in data['streams'] if s['codec_type'] == 'audio']
    video = [s for s in data['streams'] if s['codec_type'] == 'video']
    if len(audio) != 1 or len(video) != 1:
        raise RuntimeError('recording requires exactly one actual microphone track and one video track')
    frames = data.get('frames', [])
    af = [f for f in frames if f['media_type'] == 'audio' and int(f.get('nb_samples', 0)) > 0]
    vf = [f for f in frames if f['media_type'] == 'video']
    if not af or not vf:
        raise RuntimeError('recording has no decoded microphone samples or video frames')
    received, reference = decode_audio(path), decode_audio(source)
    audio_start = float(af[0]['best_effort_timestamp_time'])
    result = match_source(received, reference, expected_offset - audio_start, score_start, score_end)
    result['source_to_movie_offset_s'] = result['source_to_audio_offset_s'] + audio_start
    mapped_start = result['source_to_movie_offset_s'] + result['source_to_audio_scale'] * score_start
    mapped_end = result['source_to_movie_offset_s'] + result['source_to_audio_scale'] * score_end
    rate = int(audio[0]['sample_rate'])
    audio_end = float(af[-1]['best_effort_timestamp_time']) + int(af[-1]['nb_samples']) / rate
    video_start = float(vf[0]['best_effort_timestamp_time'])
    video_end = float(vf[-1]['best_effort_timestamp_time']) + float(vf[-1].get('pkt_duration_time', 0))
    gaps = [float(b['best_effort_timestamp_time']) - (float(a['best_effort_timestamp_time']) + int(a['nb_samples']) / rate)
            for a, b in zip(af, af[1:]) if float(a['best_effort_timestamp_time']) <= mapped_end and float(b['best_effort_timestamp_time']) >= mapped_start]
    covered = min(audio_end, video_end) >= mapped_end and max(audio_start, video_start) <= mapped_start
    result.update(streams=data['streams'], decoded_audio_samples=int(sum(int(f['nb_samples']) for f in af)),
                  decoded_mono_samples=len(received), video_frames=len(vf),
                  audio_pts_range_s=[audio_start, audio_end], video_pts_range_s=[video_start, video_end],
                  scored_movie_range_s=[mapped_start, mapped_end], scored_coverage=covered,
                  maximum_audio_gap_s=max(gaps, default=0), original_path=str(path),
                  original_sha256=hashlib.sha256(Path(path).read_bytes()).hexdigest())
    result['passed'] = result['passed'] and covered and max(gaps, default=0) <= .025
    return result
