# T1 baseline bind — 22 September 2026

No target was opened. No reset. No programming.

The pair firmware on disk is the firmware the records describe. A second check found the board's application port, and the board is running the 21 September WS2812 image, not the pair. Debugging either of those files at source level is not possible with the files that exist, and neither was rebuilt.

## Identity

- Branch `lane/k1-ra8p1-002`, commit `431140853dd8b58af53240ef84d5fb1a08bd8b45`, 141 dirty entries left in place, no stash.
- Platform memory check: valid records, sha256 `fd316def0e4e9ac5ebff549de48c306442fe968b8a758ff00f9b6127c9366ea0`. Topics recalled: pins, dma, ws2816. That check is not a hardware verdict.

## Files that match

Recomputed and matched:

- Candidate HEX `f4d40e9f7ad2733ec2ba1a70122c795801ab29f9b43a4981148b9602c3b756e7`
- Candidate ELF `459da26e8c67ee47baeaf6ecb69a230eb5170b7a31ff9064254bd2522ff4216e`
- Candidate MAP `f6aa37978b447b4497821a3b1e1b4208aaff32a062535f89ea478c96676be0d2`
- WS2812 recovery HEX `cbcadb41de10cb7f884edb38118d8ad58967f2edfe7c5afcd1eda5e62828dd37`
- WS2812 recovery ELF `646ea527901c50b9f673c1ef42b075a3584d8741f2f0342e6960e863920961d3`
- WS2812 recovery MAP `f3f35a84dd09085181c7665680be7d8cb15e5baca817c78d3be67f440cb5073d`

The build receipt has `pass: true` and `debug: false`. It has no execute field. The manifest's dry-run record has `execute: false`. The external dry receipt `programme-ws2816-pair-dry-20260921-01` has `dry_run: true` and the same build id and HEX hash. The live directory `programme-ws2816-pair-20260921-01` does not exist.

The dry receipt's restore path still names an older P3 HEX. The programmer does not roll back by itself. That path is not a WS2816 recovery.

All 115 source files named by the build receipt match the working tree byte for byte. Host tests on this tree therefore test the same sources as the retained binary.

## What is on USB

- E2 Lite is present, serial `E2L: 5AS079228B`.
- First check: Titan application port `0x045B:0x5310` was absent. Second check: it is present. See the identity section below.
- Also present: USB serial `20731830` and an Espressif JTAG/serial unit. Neither is the Titan application port.
- The control broker is not running. Nothing was killed.

A second check found the Titan application port. INFO was read once and the port was closed.

Resident identity, from the board:

- Port `/dev/cu.usbmodem00000000000011`, Renesas CDC, product "CDC USB Demonstration"
- UID `545433931bd25436593630352d068363`
- Build `90e1b84f5665008fa262d22bd9882deb5f11981542b7b48fb30edba61f8578bd`
- That build is the retained 21 September image `live-k1-runtime-build-20260921-01` (HEX `cbcadb41…`, ELF `646ea527…`). It is the 24-bit WS2812 show, not the WS2816 pair (`fa54c781…`).
- Contract `sr24000.hop180.bins80.xover40`. Runtime kind `live`. CPU1 inactive. U55 not opened.
- The reader marked it identified, and not an accepted campaign checkpoint, because no checkpoint was supplied. That is the correct result for a first bind.
- Raw reply: `handoff-info.json` in this directory.

The matching ELF for any debug session on what is running now is the 21 September recovery ELF, not the pair candidate. That ELF also has no `.debug_info` or `.debug_line`. Programming stays stopped until Rearm and the P603 check.

## Debug information in the candidate

`arm-none-eabi-readelf -S` on the candidate ELF shows `.debug_frame` and `.debug_line_str` only. There is no `.debug_info` and no `.debug_line`. The build used `-O2` and release mode. Turning on `--debug` in `scripts/build_scalar.py` forces `-O0` and a new build id. That rebuild was not done, and this ELF was not replaced.

GDB exists at the Renesas e2 studio Arm toolchain. It was not connected. The running image is now known, and that ELF has no source-level debug info.

## Allocation, from the candidate's own build log and map

- First half: P601, GPT6, DMAC0, stop on GPT0.
- Second half: P603, GPT7, DMAC3, stop on GPT1.
- Microphone: rise on DMAC1, fall on DMAC2.
- Those `-D` flags are on the compile lines in the candidate `build.log`.
- Map: `pending_a1` `0x22057940`, `pending_a0` `0x22057b20`, `owned_a1` `0x22057d00`, `owned_a0` `0x22057ee0`, `duty_a1` `0x220580c0`, `duty_a0` `0x2205bcc0`. Each duty buffer is 15360 bytes. Both sit inside the SRAM window `0x22000000`–`0x22174000`.

## Host tests (exit 0, PASS line in each log)

- `scripts/test_ws2816_pack.py`
- `scripts/test_ws2816_pair_pipeline.py`
- `scripts/test_ws281x_gpt_dma.py`
- `scripts/test_ws281x_gpt_dma_pair.py`
- `scripts/test_ws281x_gpt_dma_hw.py`
- `scripts/test_ws281x_gpt_dma_hw_pair.py`

## Not done, and why

- Physical move of DIN-B from P004 / U18 pin 16 to P603 / U18 pin 33. The wiring note still says that move is outstanding. The pair will not be run on one lane to dodge it.
- Programming. The saved command is a dry run. It was not executed, because a dry run would create the live output directory and spoil the real run.
