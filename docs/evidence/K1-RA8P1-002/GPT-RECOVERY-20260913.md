# GPT recovery checkpoint — 2026-09-13

Status: programmed and engine checkpoint PASS on the identified Titan. DMA IRQs, hardware timer-stop and latched frames advanced together with USB alive. Waveform and optical evidence remain open. G4 was not run on this image.

## Implemented

- Release ELC module-stop before ELSR/ELCON access. RA8P1 hardware manual R01UH1064EJ0110 Rev.1.10 section 20.4.3 requires this. The old driver omitted it. Whether it explains all of the former zero-frame failures awaits this image's target evidence.
- Reset timers before loading distinct first/second duties. Set GTIOA[4] for the first HIGH pulse while retaining OADFLT=LOW at stop. See manual sections 23.2.14, 23.2.17 and FSP R_GPT_Reset buffer-clear semantics. Do not write the old GPT GTINTAD low-byte interrupt-enable bits: RA8P1 reserves that byte (section 23.2.15).
- Retain actual DMA IRQ, GPT stop IRQ and reset-completed frame counters separately. GPT ISR observes GPT6 CST before CPU cleanup, requires exhausted DMA, and polling requires the DMA IRQ before crediting a frame. IRQ owner changes fail. A fault latches and ends resubmission storms.
- Add versioned binary opcode 22 with first-transaction armed, started, DMA, stop and first-fault register snapshots. It includes event routes, DMA remaining count/address, timer count/configuration, module-stop, pin selection and IRQ ownership. No printf in those interrupts.
- Add explicit --palette-gpt-dma standalone WS2812 build. The on-device palette loop copies a 128-pixel frame into the driver, services completion asynchronously, and keeps the latest pending render. GPIO commands cannot steal P601 in this image. Other image configurations retain their GPIO lane. P004 remains outside this GPT backend.
- The target scorer requires advancing real DMA interrupts, observed hardware stops and latched frames, zero faults, exact UID/build/source before and after an interval with no host frame commands. It records diagnostics on engine failures and closes CDC. G4 is NOT_RUN_THIS_IMAGE; waveform and optical evidence stay open.
- The programmer emits timestamped live stages, refuses retired HEX 42bd1d6d..., and leaves all finger instructions to the chat. Optional app-port observation after verify lets the agent avoid repeating a reset already performed. App-port detection is not image identity: run the bound scorer.
- Root AGENTS and the current canon operator table now require live chat progress. Historical LED2 and microphone receipts were not rewritten. Unrelated dirty work, the pinned BSP and DualMCU reference remain preserved; no commit was created.

## Validation

Production C driver host test: 11 scenarios PASS. Target decoder/scorer: positive and ten negative cases PASS. Programmer wrapper: success, wrong UID, readback failure and retired image PASS. Three production mutations (missing ELC module start, lost first pulse, frame credited before DMA IRQ) were rejected by runtime assertions. Existing GPT model, WS281x diagnostics, WS2816 packing and emit protocol tests PASS. Session canon regression PASS. Final target cross-build, source-manifest comparison, selected diff checks and programme dry-run PASS.

The host substitutes model registers and FSP boundaries; they do not prove signal timing. The first build compiled but hit a GPIO-symbol assertion that was inapplicable to a GPT-only image. That assertion is now conditional. Final formatting cleanup was rebuilt into the final identity below. None of the intermediate images was programmed.

## Exact prepared candidate

{
  "build_id": "a80c5d690cc473e02ba5772e357ff5269db597cbe63d4456384109cdfb19860e",
  "hex_sha256": "8cf8414c698112badcc14c916100048cdd1b7c45aa4dfe8c06651d6321d72c2d",
  "elf_sha256": "48a77c6ed995e078c5858862b38facab58c17829e132c3d8b237d781c27373e2",
  "dcache": "disabled"
}

Full execution commands and wrapper hashes: /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/gpt-recovery-source-20260913-01/rearm-ready.json

## Next operation — already prepared

On Captain's Rearm, start the prepared command before replying WAITING. Relay ROM_SEEN and actual ROM_IDENTIFIED_WRITING promptly. At WRITE_VERIFIED give one release/reset instruction only if still required. If APP_CDC_SEEN or Captain has already reset, skip that instruction and run the exact target command. Do not wait silently for process exit. Do not request another flash of this image if the engine fails; interpret the retained snapshots and make a discriminating repair first.

## Silicon result — 2026-09-13 Rearm

Programme receipt `gpt-recovery-prog-20260913-01` PASS. UID `545433931bd25436593630352d068363`. HEX `8cf8414c…` write-verified. App CDC returned without a second reset instruction.

Target receipt `gpt-recovery-run-20260913-01` PASS. Identity matched before and after a 5 s interval with no host pixel commands. Delta: 607 DMA IRQs, 607 hardware stops, 607 latched frames. Errors 0. First-fault 0. GPT clock 300 MHz. 3072-bit WS2812 frames. Stop snapshot had GPT6 CST=0 and DMA remaining 0. GTIOR 281 has initial-HIGH bit 4 set. CDC released. Palette JSON still names `gpio_diagnostic`; GPIO opcodes stay rejected on this image and the counted path is GPT/DMA.

This closes the engine checkpoint only. `WAVEFORM_NOT_CAPTURED`. Photons `NOT_CLAIMED`. `g4: NOT_RUN_THIS_IMAGE`. Do not inherit microphone, G4, WS2816 or combined-product qualification.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-13 | agent | Rearm: write verified; engine checkpoint PASS (607 DMA/stop/frames). |
