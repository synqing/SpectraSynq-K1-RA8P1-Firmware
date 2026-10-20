"""Product v9 must draw the scored path-P pixels, gains included.

The product image defines K1_INHOUSE_MAPPING_V9 and does not define
K1_INHOUSE_V9_HOST_VARIANTS. Palette and deposit gains are the compile-time
constants. The scored PAL / PAL-HD / C2 / C3 replays (replay-pfix/P) defined the
host switch and passed ``emphasis none 0.580092 0.830949 mapped``. Those two
gain strings are ``%.6f`` of calibration-P-fix.json, parsed by strtof.

This test builds the product configuration (host variants off) and the host
configuration, runs the host side with that exact argv, and requires every
output pixel of every frame to match, on both boards, for the BOOM file, the
BOOM room, Sonic room and Dopex room.

Planting the old firmware gains (0.395878 / 0.591745) back into the constants
makes the product side dimmer than the scored argv. The frames then differ and
this test goes red. Restore the replay gains afterwards.
"""
from __future__ import annotations

import hashlib
import json
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import p6_effect_mapping as mapping  # noqa: E402

INPUTS = ROOT / "_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted/replay-records/inputs.json"
CACHE = Path("/tmp/p6-product-path-p-inputs")
SONGS = ("boom_source", "boom_room", "sonic_room", "dopex_room")
# Exact argv recorded by replay-pfix/P. "default" would hide a gain drift.
HOST_V9 = ["emphasis", "none", "0.580092", "0.830949", "mapped"]
PIXEL_BYTES = 6
PIXELS_PER_FRAME = 160 * 2  # board 0, then board 1
# Frame counts are fixed by the audio length and the 10 ms render. The scored
# path-P replay (pal-P-none.replay.json) recorded these sizes; gains do not change them.
EXPECTED_BYTES = {
    "boom_source": 15934080,
    "boom_room": 18892800,
    "sonic_room": 9014400,
    "dopex_room": 18913920,
}


def _song_files() -> dict[str, Path]:
    manifest = json.loads(INPUTS.read_text())
    CACHE.mkdir(parents=True, exist_ok=True)
    out: dict[str, Path] = {}
    for name in SONGS:
        spec = manifest["inputs"][name]
        meta = spec["files"][name]
        dest = CACHE / meta["file"]
        digest = meta["sha256"]
        if dest.is_file() and hashlib.sha256(dest.read_bytes()).hexdigest() == digest:
            out[name] = dest
            continue
        if name == "boom_source":
            pcm = mapping.np.fromfile(mapping.SRC, dtype="<i2").astype(mapping.np.float64) / 32768.0
        else:
            pcm, offset, scale = mapping.room_pcm(spec["song"])
            assert offset == spec["offset_s"] and scale == spec["scale"]
        mapping.write_s16(pcm, spec["gain"], dest)
        got = hashlib.sha256(dest.read_bytes()).hexdigest()
        if got != digest:
            dest.unlink(missing_ok=True)
            raise AssertionError(f"{name} regenerated sha256 {got} != recorded {digest}")
        out[name] = dest
    return out


def _diff(product: bytes, host: bytes) -> str:
    if len(product) != len(host) or len(product) % (PIXELS_PER_FRAME * PIXEL_BYTES) != 0:
        return f"byte length {len(product)} vs {len(host)}"
    frames = len(product) // (PIXELS_PER_FRAME * PIXEL_BYTES)
    mismatches = 0
    first = None
    step = PIXEL_BYTES
    for index in range(0, len(product), step):
        if product[index:index + step] != host[index:index + step]:
            mismatches += 1
            if first is None:
                pixel = index // step
                frame = pixel // PIXELS_PER_FRAME
                within = pixel % PIXELS_PER_FRAME
                first = {"frame": frame, "board": within // 160, "pixel": within % 160}
    return f"frames={frames} pixels={frames * PIXELS_PER_FRAME} mismatches={mismatches} first={first}"


def test_product_path_p_matches_host_palette_p(tmp_path):
    songs = _song_files()
    product = tmp_path / "product"
    host = tmp_path / "host"
    mapping.build("v9-product", product)
    mapping.build("v9", host)

    def once(tag: str, exe: Path, song: str, v9: list[str] | None) -> bytes:
        frames = tmp_path / f"{tag}-{song}.bin"
        mapping.run_probe(exe, songs[song], tmp_path / f"{tag}-{song}.csv", frames, v9)
        return frames.read_bytes()

    jobs = []
    with ThreadPoolExecutor(max_workers=8) as pool:
        for song in SONGS:
            jobs.append((song, pool.submit(once, "product", product, song, None),
                         pool.submit(once, "host", host, song, HOST_V9)))
        reports = []
        for song, product_job, host_job in jobs:
            product_bytes, host_bytes = product_job.result(), host_job.result()
            assert len(product_bytes) == EXPECTED_BYTES[song], f"{song} product bytes {len(product_bytes)}"
            report = f"{song}: {_diff(product_bytes, host_bytes)}"
            reports.append(report)
            assert "mismatches=0" in report, report
    print("\n".join(reports))
