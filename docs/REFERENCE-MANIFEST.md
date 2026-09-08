# Reference Manifest — K1-RA8P1-001

## SpectraSynq behavioural/reference source

- Local path: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware`
- Remote: `https://github.com/synqing/SpectraSynq-K1-DualMCU-Firmware.git`
- Required branch at bootstrap: `lane/k1-dm-142-beat-render-scheduler`
- Pinned authority commit: `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`
- K1-DM-142 code commit beneath it: `dbdff67fcaa00c6264d4f8ad4e0202cbb1d07677`
- Role: behavioural, algorithmic and timing-semantics reference. READ ONLY.

## Titan Mini vendor/platform reference

- Local path: `/Users/spectrasynq/Workspace_Management/Software/sdk-bsp-ra8p1-titan-mini`
- Remote: `https://github.com/RT-Thread-Studio/sdk-bsp-ra8p1-titan-mini.git`
- Pinned bootstrap commit: `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`
- Role: board support, startup, FSP/RT-Thread integration, drivers and known-working Titan examples. READ ONLY.

## Especially relevant BSP examples

- `project/Titan_Mini_template` — minimum project scaffold
- `project/Titan_Mini_pdm` — board PDM/audio capture reference
- `project/Titan_Mini_wavplayer` — audio dataflow/playback reference
- `project/Titan_Mini_rpmsg` — M85/M33 multicore reference; **study only, M33 remains parked**
- `project/Titan_Mini_usb_pcdc` — USB CDC bring-up reference
- `project/Titan_Mini_driver_all` — peripheral inventory/reference

The CLI agent must run `scripts/check_references.sh` before importing or comparing source. If a reference head has moved, do not silently follow it; use the pinned commit or document an explicit authority update.
