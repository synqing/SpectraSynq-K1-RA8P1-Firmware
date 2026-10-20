# Release puppet on the Titan — 15 Oct 2026

**Image:** TEST IMAGE host-008-puppet.
- Build bbf8083d. Lane commits 302a47f, 1801c67; 217/217 receipt sources match.
- Flashed with the NOATTACH cycle. Identity matched, and the readback was 127 fields with 0 differences.
- Your palettes 8 (board 0) and 37 (board 1) were put back live.

**What it does:**
- Board 0 speed 0.501 arms the scored R01+R03 release on both edges.
- The drop lands 6 s later.
- Unarmed, it draws the product pixels (tests/host/test_p6_release_puppet.py).

**Instrument:**
- The iPhone films the plate, and the MacBook Pro microphone records the room audio into the same file.
- Playback is BOOM 40–75 s with timing chirps, played through the Bose Mini II over Bluetooth at afplay gain 0.5.
- `scripts/titan_release_timing.py` places the drop on the video clock from the chirps and finds the release as the plate-centre half-rise.
- Self-test: a known +40 ms reads +34 ms, and a planted +100 ms audio shift moves the result by exactly −100 ms.
- Limits:
  - 30 fps, so about ±17 ms per reading.
  - Only the first chirp is detected reliably; the later ones score 0.07–0.14. The detected chirps sit 0.42–0.49 s apart instead of 0.50, an unexplained effect of the speaker or room.
  - Audio and video alignment inside avfoundation is assumed, not measured.

| Take | Arm offset | Light minus sound | Plate-centre contrast | Note |
|---|---|---|---|---|
| puppet-cal-1 | 0 | not measurable | 0.21 | gain 0.25, no chirps. Waveform correlation is noise (r ≈ 0.1) and the camera stalled 4–9 s. The strip still shows the centre flooding after the drop. |
| puppet-cal-2 | 0 | −214 ms | 0.60 | Edges only through the riser, then the whole plate floods in two frames, holds about 0.6 s and falls. |
| puppet-timed-3 | +0.214 s | −31 ms | 0.59 | Calibrated: within one frame, and under the about 45 ms light-lead detectability threshold. |

**Calibration:**
- Bluetooth playback plus afplay start-up put the sound about 0.21 s behind the host clock.
- Delaying the arm by 0.214 s puts the release on the heard drop.

A blind A/B (one armed take, one plain, in a random order recorded before playback) was run for your verdict. Its files are added after you answer.
