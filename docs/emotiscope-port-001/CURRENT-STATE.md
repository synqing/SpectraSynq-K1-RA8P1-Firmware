# Titan Emotiscope port — current execution record

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

## Scope and authority

The user's 29 September 2026 pivot parks the bespoke Titan AP/VP and ports the known Emotiscope.HIL audio and visual engines. The original effects worktree, rejected appearances, original contracts, Aurora A image and unprogrammed Aurora B image remain preserved. This execution does not close the old AP/VP/CTL/IVV contracts or claim operator appearance acceptance.

Implementation worktree: /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001
Branch: feat/titan-emotiscope-port-001.
Parent commit: 4ced4d1d0001ba22788759eb3318a73179f94d81.

## What is running

Final programmed build: 72773a7461590edbdb53080670f6c61608fb973c889f7bdcc1b9a65da4b8319b.
HEX SHA256: 28acf53a54259ac6f8e8d0a2ef3e6fda7aa5d513859310f757de3ba2edd815eb.
ELF SHA256: d1d446c6f4db432a0b1e4cd53f7df8fb9b437e2057ec663d2ed9be91b4693fb7.
Donor: Emotiscope.HIL/v1.1_build at 7f86939bd7c16c4a6dd5789813e9d551246ed134.
Schema: c7b67f1eecbe836721c97331eef895813857191e273999c96b1f3fbf41bce39a.

Boot is autonomous emission ON, donor Spectrum mode 1, hardware brightness 100/255. The native donor default colour is retained. The donor's startup fade is retained; it reaches full master gain after approximately eleven seconds at the explicit 100 Hz port cadence.

All seven active donor modes are present: Analog, Spectrum, Octave, Metronome, Spectronome, Hype and Bloom. Network, ESP32 RMT, touch hardware and NVS services are replaced or excluded. This is a complete donor musical AP/VP port, not an ESP32 peripheral emulation.

## Active path

Titan PDM capture and original per-block microphone loud guard -> 95-tap antialias filter -> measured-source-rate conversion directly to 12800 Hz, 64 samples -> original donor Goertzel, chroma, VU, tempo -> complete donor effect and colour/output processing -> donor gamma and temporal RGB8 dithering -> nearest original pixel mapping from 128 to 160 -> RGB8 expansion for GRB48 -> exactly one hardware brightness gain.

One continuous 160-pixel scene is split at logical 79/80. DIN-A carries 0–79 on P601/GPT6/DMAC0; DIN-B carries 80–159 on P604/GPT8/DMAC3. The microphone retains DMA1/2. No second strip or independent second-edge scene is claimed. This is RGB8 donor output carried by a 16-bit wire format, not authored TRUE16.

Compiled call-site checks confirm that LiveAudioRuntime calls donor AP and PaletteRuntime calls donor VP; those owners do not call legacy AP or product rendering. Existing compatibility source is still staged where required by the fixture framework.

## Declared adaptations

- Native 128-pixel donor geometry remains inside the engine. Spatial expansion occurs only after its final RGB8 output.
- Explicit 100 Hz rendering is a port choice, not a recovered historical S3 frame rate.
- Original tempo full-window index -1 was repaired in two expressions. The pristine original and restored-defect mutation both fail AddressSanitizer.
- Novelty deadline comparison is wrap-safe. Extended milliseconds preserve time across microsecond rollover; backward execution timestamps are rejected. AP and VP share the same monotonic target clock.
- A capture discontinuity clears only the PCM capture history, invalidates fresh audio and rewarms 64 chunks/320 ms. Visual, colour, phase, configuration and dither histories remain. This preserves prior material; it does not reconstruct missing audio.
- ARM platform shims preserve scalar DSP operations and define M_PI. No invented pitch estimator or K1 AP feature substitution drives the donor.
- Donor chroma is preserved in its actual D#/Eb-origin ordering. Unsupported legacy beat, onset, saliency and tempo-field claims stay unset.

## Verification

Source-bound host evidence:
- out/emotiscope-ap-parity-002/result.json: 34000 AP hops, two 75-second trance fixtures plus synthetic input; 193505382 scalar AP comparisons agree exactly.
- out/emotiscope-visual-verification-003/result.json: all active modes, independent Spectrum output-stage and dither oracle, gap preservation and clock rollover. Five deliberately broken variants rejected.
- out/emotiscope-output-001/result.json: all 960000 wire bytes match an independent donor/output calculation over 1000 frames. Asymmetric markers cover all source pixels, endpoints, centre pair and 80+80 split. Wrong split, duplicate gain and second gamma variants rejected.
- evidence/live-bridge-host-002/receipt.json: latest wrapper, contract checks, 64-hop warmup, 5000 us deadline, 240 media ticks/hop and recovery. Three broken bridge variants rejected.
- evidence/capture-adapter-host-01.json: direct 12.8 kHz adapter, sample accounting, sanitizer and filter checks.
- evidence/legacy-asrc-byte-parity-01.json: original 24 kHz profile preserved over 2368000 samples.

ARM 001 retains its missing-M_PI compile failure. ARM 002 is superseded compile evidence with legacy schema. ARM 003 was programmed and passed initial live and Sonic observations. ARM 004 changes only the two diagnostic descriptions for capture rate/profile and physical backend pins. Its active donor and schema are identical to 003.

Exact-image build: evidence/arm-port-004/receipt.json.
Exact-image programme: evidence/programme-port-004/receipt.json.
Target observations: evidence/target-sonic-004/result.json and evidence/target-carte-004/result.json.
The final measured aggregate is in evidence/target-summary-004.json.

Programming used E2 Lite 5AS079228B, SWD 1 MHz, probe power OFF, -noquery -run, and the existing permitted range ending before configuration memory. The application returned as UID 545433931bd25436593630352d068363 and the exact build above.

## Controls and continued work

For this profile opcode 16 version 1 selects donor mode 0–6 using the existing queued owner mechanism. Send eight little-endian uint32 words: version=1, palette_a=0, palette_b=0, mode_a=mode, mode_b=mode, flags=5, brightness=100, output_channel=0. Preserve emission. Explicit operator dimming remains possible; the normal boot default stays at least 100.

Legacy opcode 25 performance/config controls are deliberately rejected because they describe the parked engine. The new schema states this. A K1 slider/control integration is not claimed. Use one CDC owner and the existing lease; release the serial handle and its flock when finished.

The implementation and physical deployment phase is complete when the exact image is identified and the bounded tests recorded. Visual acceptance remains the operator's decision on the actual plate. No software counter is optical proof. Long-duration unattended continuity, absolute acoustic timing and full calibration are not established by these short observations.

The old source and rollback identities are under evidence/parked-20260929. Restore them only for a concrete recovery need or a subsequent operator instruction; do not overwrite or reinterpret their historical receipts.
