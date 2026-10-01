# Titan Emotiscope port — current execution record

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

## Current image: palettes drawn the way K1 drew them — 1 October 2026

Resident since 16:48 AWST: build `05d016a7f1232ab389379d9c1bd9a1344e9ee5e292a569b8fca9affebe3b260c`, HEX `349916e714cea1806fd78d3a9b8a87f6b35214f80b239d944027622c50f816ab`, ELF `dadafbee814ea8b544326ab6c4985333162a4e89b5713870551f602459a475dc`, schema unchanged `e40f6598…`, same UID, pins, DMA (LED 0/3, mic 1/2) and 80+80 split. Build `evidence/arm-palette-002`, programme `evidence/programme-palette-002`, admission `evidence/smc-palette-002/accepted-candidate.json` (broker LaunchAgent points here).

Why: the palette data was byte-identical to K1 (44/44 stop tables), but the Titan drew it differently: the donor's final x² gamma (K1 had `ENABLE_OUTPUT_GAMMA 0` since 20 May, "crushes midtones"), only a Colour-range slice of the palette, and a saturation grey blend. Modelled on the Captain's settings this halved the visible colours (9.9 → 4.8 per strip) and made half-loud passages 3.2× darker. Comparison page: https://claude.ai/artifact/X31AHA1nkSFMeUxt6qDaVG.

Now, for palettes 1–44 only: palette position = Colour + strip position (reverse mirrors it), level = 0.65v²+0.35v (K1 SQUARE_ITER 1), no saturation blend, final gamma skipped. Palette 0 is the donor unchanged (precision-0 output still byte-identical to the reference). The leds.h background keeps the donor hue slice (background defaults to 0). Host: Colour range and Saturation show "No effect with a palette on".

Checks: `k1palette` host case, 2,016 pixels against an independent K1 oracle, worst error 1.2e-7; planted faults (gamma kept, colour range used, linear level) each rejected; `scripts/check_instrument.sh` PASS; receipt sources 194/194 match HEAD `07f5bc2`. On the board: identity MATCH, 100 frames/s, 0 LED faults; palette 31 PATCH accepted and rendered. No LED-frame capture was taken (the wire snapshot needs the show stopped), so on-board colour is not separately measured. The show stopped about 18 s during programming.

## Previous image: K1 palettes restored — 30 September 2026 — superseded above

Resident: build `5cbd964d1494bc9480333206b367006326e8e3fad44df6a86f81a0dd3985cbef`, HEX `3e89dd282aff0d0c2b464299f756043175beb3a76ef7907bb5a87bf5154a2813`, ELF `5004027784351cffcaefcf2b64b4b5fbb84c57c8803badd142bf10e893f833ed`, schema `e40f6598e57c9493de6f3e6d19935d404d9f5cdbb35c064770fe365ad476a0bb`, same UID, pins, DMA and 80+80 split. Build `evidence/arm-palette-001`, programme `evidence/programme-palette-001`, admission `evidence/smc-palette-001/accepted-candidate.json` (sha `48bd7894…`, used by the broker LaunchAgent).

Why: the Emotiscope port had dropped the K1 44-palette catalogue, so every mode could only use the donor's plain HSV hues. The donor `hsv()` call is now routed through `sampleProductPaletteHd` when a palette is selected. New PATCH field 18 `palette` (0–44, GET state offset 140): 0 = native Emotiscope colour (byte-identical to before), 1–44 = the K1 product palettes. Colour range spreads how much of the palette shows; saturation still blends toward grey. Boot palette 0. Round trip 0→1→0 accepted and rendered on the board. Host: Knob 2 sweeps all 44, Up/Down step one, the surface page shows the palette name. Output check `palette25-changes-wire`: 140,890 of 960,000 bytes differ from palette 0.

The show stopped for about 25 s during programming and resumed on application start.

## Previous image, supervision and A/B — 30 September 2026 (Dependable Instrument v1) — superseded above

Resident since 06:57 AWST: build `c23e1b61f1de37fdc5179548ef2aa350921be23bb9728ab05d3644f6e08acb74`, HEX `5337d52473a599eb42faa3acbf237dcdce1b1756b8d814c1d572a5fecdb10448`, ELF `89582d0013d8da2ec064cf8bbf61aac1b47fce7f02782c565213ca1411bfaf20`, schema `407d24207fa2a7f14c9ccf34ed601bc94bab1e873fc47be3e83850c1c1924688`, UID `545433931bd25436593630352d068363`. Boots Spectrum, 100/255, emission on, `output_precision` 0. Same P601/GPT6/DMA0, P604/GPT8/DMA3, mic DMA1/2 and 80+80 split. Build `evidence/arm-precision-001`, programme `evidence/programme-precision-001`, admission `evidence/smc-precision-001/accepted-candidate.json` (sha `3d494442…`).

`output_precision` (PATCH field 17, GET flag bit 6): 0 = reference RGB8 dithered ×257 (unchanged; byte-identical to `titan-emotiscope-ref-r1` over the 1000-frame fixture), 1 = the same post-gamma floats rounded to 16 bits. Owner-applied at a frame boundary; never in scenes/defaults; reboot returns 0. Round trip 0→1→0 verified on the board (`evidence/instrument-v1-precision-01`). No A/B verdict yet.

Broker and mixer controller run under launchd (`com.spectrasynq.titan-broker`, `com.spectrasynq.titan-mixer`); commands are on the operator card. The broker exits when Titan's USB node vanishes so launchd re-attaches it. Crash (SIGKILL) and bootout/bootstrap tests passed with frames advancing throughout (`evidence/instrument-v1-launchd-02`). One honest check: `scripts/check_instrument.sh`.

Fault observed 06:17–06:54: the USB branch shared by Titan, keypad, AV adapter, ESP32 and MD202 dropped twice; Titan lost power once (rebooted 06:18) and its USB then stayed absent until an SWD identity read restarted the application. Cause not established (bench cabling or hub). Evidence `evidence/instrument-v1-launchd-01`, `evidence/instrument-v1-titan-usb-01`.

## Previous controls image and host — superseded 30 September 2026

Updated 29 September 2026 from the running broker/controller, not from Git HEAD.

Resident build `393527e4ff81eb5c4fe54e8be3bf874bc37dce9fd89a2d9fec3d4b3e7ac8c4cb`; HEX SHA-256 `73ad9be5401110a06f1e7f728bf592c34c4e6170d147b1c4faaaf7831cf30071`; ELF SHA-256 `ed997e83fee4da3cde1f23fe6acdc49e41e184cd01e71f68e274d9ba3790849f`; schema `5f3c9fa627067bf3357bdb142b4d4efcb8d16211164431864826af9102286f60`. UID `545433931bd25436593630352d068363`. Spectrum, output100/255 and emissionON were verified at the baseline/restoration. Subsequent live Knob1 input set output to255/255; that operator choice was preserved. Normal default100 is unchanged. The earlier port004 image below is preserved history, not current firmware.

Current control interface: typed Emotiscope GET/PATCH under opcode25. Legacy opcode16 mutation and the old performance config are rejected. **Do not use the historical eight-word opcode16 instructions below on this image.** Continuous160/80+80 mapping, P601/P604, DMA allocation and the donor AP/VP are unchanged.

The controller is running on the existing admitted SMC-Mixer Bluetooth endpoint. Dashboard8776, status8767, existing sole broker socket `/tmp/titan-emotiscope-control/titan-control.sock`. No second flash occurred during Captain's host repair. A real synthetic softness change and exact restoration reached accepted/rendered/completed target state. This is not a human gesture or optical result.

155 complete-collection host tests passed in the documented existing-dependency environment; 39 focused checks passed in the production virtualenv. Full physical control qualification is still open. Fader1/Fader4 remain gated. The first 20-second host window had no input; subsequent real mixer CC traffic and advancing accepted/rendered/completed state are retained in the repair evidence. Capture-finalisation hardening remains outstanding. Details, failing historical logs and exact runtime invocation: `evidence/captain-host-repair-20260929-01/CLOSEOUT.md`. Operator card: SMC worktree `host/titan_control/EMOTISCOPE-OPERATOR-20260929.md`.

## Scope and authority

The user's 29 September 2026 pivot parks the bespoke Titan AP/VP and ports the known Emotiscope.HIL audio and visual engines. The original effects worktree, rejected appearances, original contracts, Aurora A image and unprogrammed Aurora B image remain preserved. This execution does not close the old AP/VP/CTL/IVV contracts or claim operator appearance acceptance.

Implementation worktree: /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001
Branch: feat/titan-emotiscope-port-001.
Parent commit: 4ced4d1d0001ba22788759eb3318a73179f94d81.

## Historical initial port image — superseded above

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

## Historical pre-controls interface — not valid for the current image

For this profile opcode 16 version 1 selects donor mode 0–6 using the existing queued owner mechanism. Send eight little-endian uint32 words: version=1, palette_a=0, palette_b=0, mode_a=mode, mode_b=mode, flags=5, brightness=100, output_channel=0. Preserve emission. Explicit operator dimming remains possible; the normal boot default stays at least 100.

Legacy opcode 25 performance/config controls are deliberately rejected because they describe the parked engine. The new schema states this. A K1 slider/control integration is not claimed. Use one CDC owner and the existing lease; release the serial handle and its flock when finished.

The implementation and physical deployment phase is complete when the exact image is identified and the bounded tests recorded. Visual acceptance remains the operator's decision on the actual plate. No software counter is optical proof. Long-duration unattended continuity, absolute acoustic timing and full calibration are not established by these short observations.

The old source and rollback identities are under evidence/parked-20260929. Restore them only for a concrete recovery need or a subsequent operator instruction; do not overwrite or reinterpret their historical receipts.
