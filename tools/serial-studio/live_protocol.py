#!/usr/bin/env python3
"""Little-endian Titan live-K1 wire codec. AudioFeaturesV1 is not on the wire."""
from __future__ import annotations

import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CONTRACT_PATH = ROOT / "docs/contracts/titan-live-v1.json"
CONTRACT = json.loads(CONTRACT_PATH.read_text())
SCHEMA_SHA256 = hashlib.sha256(CONTRACT_PATH.read_bytes()).hexdigest()

SNAP_MAGIC = 0x53564C54
EVENT_HIST_MAGIC = 0x48454C54
TIMING_HIST_MAGIC = 0x48544C54
SNAPSHOT_BYTES = 1184
EVENT_BYTES = 192
TIMING_BYTES = 56
HEADER_BYTES = 64
CONFIG_BYTES_V1 = 896
CONFIG_BYTES_V2 = 1152
CONFIG_BYTES = CONFIG_BYTES_V1
VISUAL_BYTES = 128
CFG_GET_SCHEMA = 1
CFG_GET_CONFIG = 2
CFG_SET_CONFIG = 3
CFG_BEGIN_SET = 4
CFG_APPEND_SET = 5
CFG_COMMIT_SET = 6
CFG_ABORT_SET = 7
CFG_TEST_STALE = 11


def _u16(buf: bytes, off: int) -> int:
    return struct.unpack_from("<H", buf, off)[0]


def _u32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<I", buf, off)[0]


def _u64(buf: bytes, off: int) -> int:
    return struct.unpack_from("<Q", buf, off)[0]


def _f32(buf: bytes, off: int) -> float:
    return struct.unpack_from("<f", buf, off)[0]


def unpack_snapshot(buf: bytes) -> dict:
    if len(buf) < SNAPSHOT_BYTES:
        raise ValueError("short snapshot")
    if _u32(buf, 0) != SNAP_MAGIC:
        raise ValueError("bad snapshot magic")
    chroma = [_f32(buf, 156 + i * 4) for i in range(12)]
    spectrum = [_f32(buf, 204 + i * 4) for i in range(80)]
    return {
        "version": _u16(buf, 4),
        "flags": _u16(buf, 6),
        "publication_generation": _u64(buf, 8),
        "stream_epoch": _u64(buf, 16),
        "hop_sequence": _u64(buf, 24),
        "capture_time_us": _u64(buf, 32),
        "publish_time_us": _u64(buf, 40),
        "analysis_end_exclusive": _u64(buf, 48),
        "media_time_48k": _u64(buf, 56),
        "musical_pulse": _u64(buf, 64),
        "musical_phase_q32": _u32(buf, 72),
        "tempo_bpm": _f32(buf, 76),
        "beat_phase": _f32(buf, 80),
        "beat_confidence": _f32(buf, 84),
        "predicted_next_beat_us": _u64(buf, 88),
        "peak_scaled": _f32(buf, 96),
        "vu_level": _f32(buf, 100),
        "bass_energy": _f32(buf, 104),
        "mid_energy": _f32(buf, 108),
        "high_energy": _f32(buf, 112),
        "onset_strength": _f32(buf, 116),
        "flux": _f32(buf, 120),
        "centroid_hz": _f32(buf, 124),
        "rolloff_hz": _f32(buf, 128),
        "bandwidth_hz": _f32(buf, 132),
        "rms": _f32(buf, 136),
        "crest": _f32(buf, 140),
        "zcr": _f32(buf, 144),
        "hfc": _f32(buf, 148),
        "compactness": _f32(buf, 152),
        "chroma": chroma,
        "spectrum": spectrum,
        "bass_onset": _f32(buf, 524),
        "kick": _f32(buf, 528),
        "snare": _f32(buf, 532),
        "hihat": _f32(buf, 536),
        "transient": _f32(buf, 540),
        "saliency": _f32(buf, 544),
        "harmonic_ratio": _f32(buf, 548),
        "inharmonicity": _f32(buf, 552),
        "pitch_hz": _f32(buf, 556),
        "pitch_confidence": _f32(buf, 560),
        "mode_id": _u16(buf, 596),
        "palette_id": _u16(buf, 598),
        "gain_q8": _u16(buf, 600),
        "warmup_hops": _u16(buf, 602),
        "config_revision": _u32(buf, 604),
        "hops_consumed": _u32(buf, 608),
        "hops_rejected": _u32(buf, 612),
        "last_invalidate_reason": _u32(buf, 616),
        "asrc_starved": _u32(buf, 620),
        "measured_hz": _u32(buf, 624),
        "deadline_us": _u32(buf, 628),
        "publication_deadline_misses": _u32(buf, 808) if len(buf) >= 812 else 0,
        "ap_cycles": _u32(buf, 632),
        "vp_cycles": _u32(buf, 636),
        "hop_dt_us": _u32(buf, 640),
        "event_mask": _u32(buf, 644),
        "event_write_seq": _u64(buf, 648),
        "timing_write_seq": _u64(buf, 656),
        "freshness_us": _u32(buf, 664),
        "live_origin": buf[668],
        "physical_ts_valid": buf[669],
        "emit_on": buf[670],
        "capture_start_us": _u64(buf, 672),
        "capture_end_us": _u64(buf, 680),
        "injection_start_us": _u64(buf, 688),
        "injection_end_us": _u64(buf, 696),
        "stale_entry_us": _u64(buf, 704),
        "epoch_transition_us": _u64(buf, 712),
        "warmup_complete_us": _u64(buf, 720),
        "uid": buf[728:744].hex(),
        "build_sha256": buf[744:776].hex(),
        "schema_sha256": buf[776:808].hex(),
        "valid": bool(_u16(buf, 6) & 1),
        "warming": bool(_u16(buf, 6) & 2),
        "stale": bool(_u16(buf, 6) & 4),
    }


def unpack_history(buf: bytes, rec_bytes: int, decoder) -> dict:
    if len(buf) < HEADER_BYTES:
        raise ValueError("short history")
    count = _u16(buf, 32)
    expect = HEADER_BYTES + count * rec_bytes
    if len(buf) < expect:
        raise ValueError("short history records")
    records = [decoder(buf[HEADER_BYTES + i * rec_bytes : HEADER_BYTES + (i + 1) * rec_bytes]) for i in range(count)]
    return {
        "magic": _u32(buf, 0),
        "version": _u16(buf, 4),
        "flags": _u16(buf, 6),
        "oldest_seq": _u64(buf, 8),
        "newest_seq": _u64(buf, 16),
        "requested_after": _u64(buf, 24),
        "count": count,
        "truncated": _u16(buf, 34),
        "record_bytes": _u16(buf, 36),
        "opcode": _u16(buf, 38),
        "generation": _u64(buf, 40),
        "stream_epoch": _u64(buf, 48),
        "empty": bool(_u16(buf, 6) & 1),
        "gap": bool(_u16(buf, 6) & 2),
        "expired_cursor": bool(_u16(buf, 6) & 4),
        "records": records,
    }


def unpack_event(buf: bytes) -> dict:
    if len(buf) < EVENT_BYTES:
        raise ValueError("short event")
    return {
        "write_seq": _u64(buf, 0),
        "capture_time_us": _u64(buf, 8),
        "publish_time_us": _u64(buf, 16),
        "hop_sequence": _u64(buf, 24),
        "stream_epoch": _u64(buf, 32),
        "event_mask": _u32(buf, 40),
        "onset_strength": _f32(buf, 44),
        "flux": _f32(buf, 48),
        "bass_onset": _f32(buf, 52),
        "kick": _f32(buf, 56),
        "snare": _f32(buf, 60),
        "hihat": _f32(buf, 64),
        "transient": _f32(buf, 68),
        "saliency": _f32(buf, 72),
        "peak_scaled": _f32(buf, 76),
        "beat_phase": _f32(buf, 80),
        "beat_confidence": _f32(buf, 84),
        "tempo_bpm": _f32(buf, 88),
        "mode_id": _u16(buf, 92),
        "flags": _u16(buf, 94),
        "spectrum_peak_bin": _u16(buf, 96),
        "chroma_peak": _u16(buf, 98),
    }


def unpack_timing(buf: bytes) -> dict:
    if len(buf) < TIMING_BYTES:
        raise ValueError("short timing")
    return {
        "write_seq": _u64(buf, 0),
        "hop_sequence": _u64(buf, 8),
        "capture_time_us": _u64(buf, 16),
        "publish_time_us": _u64(buf, 24),
        "ap_cycles": _u32(buf, 32),
        "vp_cycles": _u32(buf, 36),
        "hop_dt_us": _u32(buf, 40),
        "asrc_starved": _u32(buf, 44),
        "measured_hz": _u32(buf, 48),
        "flags": _u16(buf, 52),
    }


VIS_ENABLED = 1
VIS_MIRROR = 2
VIS_AUTO_COLOUR = 4
VIS_REVERSE = 8
VIS_INCANDESCENT = 16
VIS_DITHER = 32
VIS_BASE_COAT = 64
VIS_PALETTE_MODE = 128
VIS_CHROMATIC = 256
VIS_FIX_AGC = 512
VIS_FIX_CHROMA_GATE = 1024
VIS_FIX_PRISM_OFF = 2048
VIS_FIX_BLOOM_DECAY = 4096
VIS_FIX_HSV = 8192
VIS_FIX_SECONDARY = 16384
VIS_BLOOM_FORCE_SAT = 32768

_VIS_FLOATS = (
    "chroma", "mood", "saturation", "square_iterations", "sensitivity",
    "incandescent_filter", "bulb_opacity", "base_coat_intensity", "prism_count",
    "hue_position", "chroma_value", "hue_shifting_mix", "vp_bloom_alpha",
    "vp_bloom_shift_scale", "vp_waveform_shift_rate", "vp_waveform_idle_fade",
    "vp_waveform_raw_margin", "vp_waveform_peak_floor", "vp_waveform_active_fade",
    "vp_waveform_chroma_blend_gain", "vp_waveform_fallback_brightness",
    "vp_waveform_vu_floor",
)

_VIS_DEFAULT = {
    "chroma": 0.0, "mood": 0.0, "saturation": 1.0, "square_iterations": 0.0,
    "sensitivity": 1.0, "incandescent_filter": 0.0, "bulb_opacity": 0.0,
    "base_coat_intensity": 0.0, "prism_count": 0.0, "hue_position": 0.0,
    "chroma_value": 0.0, "hue_shifting_mix": 0.0, "vp_bloom_alpha": 0.99,
    "vp_bloom_shift_scale": 1.0, "vp_waveform_shift_rate": 120.0,
    "vp_waveform_idle_fade": 0.985, "vp_waveform_raw_margin": 1.10,
    "vp_waveform_peak_floor": 0.08, "vp_waveform_active_fade": 0.04,
    "vp_waveform_chroma_blend_gain": 2.0, "vp_waveform_fallback_brightness": 1.0,
    "vp_waveform_vu_floor": 0.02, "sweet_spot_min_level": 350,
    "samples_per_chunk": 180,
    "flags": VIS_ENABLED | VIS_MIRROR | VIS_CHROMATIC | VIS_BLOOM_FORCE_SAT,
}


def unpack_visual(buf: bytes, off: int = 0) -> dict:
    if len(buf) < off + VISUAL_BYTES:
        raise ValueError("short visual")
    out = {}
    for i, name in enumerate(_VIS_FLOATS):
        out[name] = _f32(buf, off + i * 4)
    out["sweet_spot_min_level"] = _u32(buf, off + 88)
    out["samples_per_chunk"] = _u16(buf, off + 92)
    out["flags"] = _u32(buf, off + 96)
    return out


def pack_visual(vis: dict | None) -> bytes:
    src = dict(_VIS_DEFAULT)
    if vis:
        src.update(vis)
    buf = bytearray(VISUAL_BYTES)
    for i, name in enumerate(_VIS_FLOATS):
        struct.pack_into("<f", buf, i * 4, float(src[name]))
    struct.pack_into("<I", buf, 88, int(src["sweet_spot_min_level"]))
    struct.pack_into("<H", buf, 92, int(src["samples_per_chunk"]))
    struct.pack_into("<I", buf, 96, int(src["flags"]))
    return bytes(buf)


def unpack_config(buf: bytes) -> dict:
    if len(buf) < CONFIG_BYTES_V1:
        raise ValueError("short config")
    focus_a = [_f32(buf, 56 + i * 4) for i in range(105)]
    focus_b = [_f32(buf, 476 + i * 4) for i in range(105)]
    out = {
        "version": _u32(buf, 0),
        "revision": _u32(buf, 4),
        "palette_version": _u32(buf, 8),
        "palette_a": _u32(buf, 12),
        "palette_b": _u32(buf, 16),
        "mode_a": _u32(buf, 20),
        "mode_b": _u32(buf, 24),
        "flags": _u32(buf, 28),
        "brightness": _u32(buf, 32),
        "output_channel": _u32(buf, 36),
        "transition_ms": _u32(buf, 40),
        "travel_ms": _u32(buf, 44),
        "emit_on": buf[48],
        "focus_a": focus_a,
        "focus_b": focus_b,
        "visual_capable": False,
        "wire_bytes": len(buf),
    }
    if len(buf) >= CONFIG_BYTES_V2 and out["version"] >= 2:
        out["visual_a"] = unpack_visual(buf, CONFIG_BYTES_V1)
        out["visual_b"] = unpack_visual(buf, CONFIG_BYTES_V1 + VISUAL_BYTES)
        out["visual_capable"] = True
    return out


def pack_config(cfg: dict) -> bytes:
    focus_a = list(cfg.get("focus_a") or [1.0] * 105)
    focus_b = list(cfg.get("focus_b") or [1.0] * 105)
    focus_a = (focus_a + [1.0] * 105)[:105]
    focus_b = (focus_b + [1.0] * 105)[:105]
    version = int(cfg.get("version", 1))
    visual_capable = bool(cfg.get("visual_capable")) or version >= 2 or (
        cfg.get("visual_a") is not None or cfg.get("visual_b") is not None
    )
    if visual_capable:
        version = max(version, 2)
    size = CONFIG_BYTES_V2 if version >= 2 else CONFIG_BYTES_V1
    buf = bytearray(size)
    struct.pack_into("<12I", buf, 0,
                     version,
                     int(cfg.get("revision", 0)),
                     int(cfg.get("palette_version", 1)),
                     int(cfg.get("palette_a", 0)),
                     int(cfg.get("palette_b", 1)),
                     int(cfg.get("mode_a", 32)),
                     int(cfg.get("mode_b", 0)),
                     int(cfg.get("flags", 1)),
                     int(cfg.get("brightness", 24)),
                     int(cfg.get("output_channel", 0)),
                     int(cfg.get("transition_ms", 0)),
                     int(cfg.get("travel_ms", 4000)))
    buf[48] = int(cfg.get("emit_on", 0))
    for i, value in enumerate(focus_a):
        struct.pack_into("<f", buf, 56 + i * 4, float(value))
    for i, value in enumerate(focus_b):
        struct.pack_into("<f", buf, 476 + i * 4, float(value))
    if version >= 2:
        buf[CONFIG_BYTES_V1:CONFIG_BYTES_V1 + VISUAL_BYTES] = pack_visual(cfg.get("visual_a"))
        buf[CONFIG_BYTES_V1 + VISUAL_BYTES:CONFIG_BYTES_V2] = pack_visual(cfg.get("visual_b"))
    return bytes(buf)


def pack_config_sub(sub: int, payload: bytes = b"") -> bytes:
    return struct.pack("<I", sub) + payload


def pack_stale_test(uid_hex: str, build_sha_hex: str) -> bytes:
    uid = bytes.fromhex(uid_hex)
    build = bytes.fromhex(build_sha_hex)
    if len(uid) != 16 or len(build) != 32:
        raise ValueError("identity width")
    return pack_config_sub(CFG_TEST_STALE, uid + build)


def pack_cursor(after: int) -> bytes:
    return struct.pack("<Q", int(after))


def unpack_schema_page(buf: bytes) -> dict:
    if len(buf) < 12:
        raise ValueError("short schema page")
    offset = _u32(buf, 0)
    total = _u32(buf, 4)
    take = _u32(buf, 8)
    body = buf[12:12 + take]
    return {"offset": offset, "total": total, "take": take, "json": body}


def groups_present(snap: dict) -> dict:
    return {
        "identity": bool(snap.get("publication_generation") is not None),
        "level": True,
        "spectrum": len(snap.get("spectrum") or []) == 80,
        "chroma": len(snap.get("chroma") or []) == 12,
        "onset": True,
        "beat": True,
        "percussion": True,
        "saliency": True,
        "musical_time": True,
        "freshness": True,
        "controls": True,
    }
