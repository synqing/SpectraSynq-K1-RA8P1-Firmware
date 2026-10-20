# WS2816 pair recovery and wiring — 21 September 2026

This is the recovery and reconnection record for candidate `WS2816-PAIR-CANDIDATE-20260921-A`. It does not report a flash.

## Resident recovery show (keep these bytes)

Do not substitute the older `p3-2026-09-09` staged-v2 HEX. That path is what `programme_scalar.py` still records as `restore`, and it is the previous known-good Titan image, not this working live-K1 show.

| Field | Value |
| --- | --- |
| Build directory | `docs/evidence/K1-RA8P1-002/live-k1-runtime-build-20260921-01/` |
| `build_id` | `90e1b84f5665008fa262d22bd9882deb5f11981542b7b48fb30edba61f8578bd` |
| HEX SHA-256 | `cbcadb41de10cb7f884edb38118d8ad58967f2edfe7c5afcd1eda5e62828dd37` |
| ELF SHA-256 | `646ea527901c50b9f673c1ef42b075a3584d8741f2f0342e6960e863920961d3` |
| Programme | `programme-20260921-02-palette-preview` WRITE_VERIFIED |
| UID | `545433931bd25436593630352d068363` |
| Output | one P601 GPT/DMA lane, profile 1, 24-bit WS2812 |
| Allocation | LED DMAC0; PDM DMAC1/DMAC2; GPT6 waveform; GPT0 stop |

Re-programming this HEX restores the resident show **only** on a compatible 24-bit WS2812 output setup. It cannot recover that visual on a substituted 48-bit WS2816 stick.

## Physical strip and connections (as of this checkpoint)

Recorded resident setup:

- One logical channel on P601 / U18 pin 7 through the existing HCT shifter.
- Second shifter input still on P004 / U18 pin 16 until the operator moves it.
- Profile 1 WS2812 timings (250 / 875 / 1250 ns, 300 µs reset).
- Boot: WaveformK1 mode 32, palettes 33/43, emit on, brightness 128.

After the stick is changed to the independent 80/80 WS2816 part, restoring the HEX above without restoring the WS2812 wiring will not produce the old show.

## Recovery command (WS2812 wiring restored first)

Power removed. Reconnect the WS2812 strip and P004 second-input path as they were for the resident image. Then, after Rearm:

```sh
python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/live-k1-runtime-build-20260921-01 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/programme-ws2812-recovery-YYYYMMDD-NN
```

Use a fresh output directory. Do not reuse a previous programme receipt. `--execute` waits on Rearm.

## Pair candidate wiring (operator, power removed)

These actions gate physical pair testing. They do not gate source work.

1. Confirm the WS2816 stick is the recorded independent 80/80 split. The LED 80→81 data connection is broken. The two inputs are not a redundant DIN/BIN pair.
2. Disconnect the second shifter input from P004 / U18 pin 16. Connect it to P603 / U18 pin 33. Keep the existing HCT conversion path, its supply, and common ground. Do not tie P004 and P603 together.
3. Confirm each shifted output reaches its intended half. Confirm polarity and supply at the stick.
4. Establish the permitted current/brightness limit before any test output. Sparse patterns do not qualify full-white supply.
5. Record a photo or wiring table and confirm: **DIN-B moved to P603; 80/81 break checked.**

Until that sentence is recorded, pair firmware may be built and dry-run programmed, but must not be treated as a physical pair test.

## Pair candidate bring-up settings (before Rearm)

- Backend: `ws2816_gpt_pair` on P601 + P603, profile 3, 160 logical pixels, two 80-pixel lanes.
- Boot: WaveformK1 mode 32, palettes 33/43, **physical emit off**, brightness 24.
- AP contract unchanged: 24 kHz / 180 samples.
- Do not emit an unrestricted diagnostic frame on boot.
- After WRITE_VERIFIED, match UID, full build identity and backend/profile through the single CDC owner before enabling emit.

## Datasheet bind

Worldsemi WS2816C-1313-4P, previously identified SHA-256 `43d21c1679762aeba371e4499b7a630c35c1afc1275005e2e736e1ba3bb9a7f1`. Profile 3 nominal values used by this candidate: T0H 250 ns, T1H 650 ns, period 1250 ns, reset 300 µs. Physical pulse claims stay open until a scored DIN capture on this image.
