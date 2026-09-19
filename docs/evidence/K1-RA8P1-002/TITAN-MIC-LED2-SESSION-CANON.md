# Titan Mini microphone and LED2 bring-up canon

Read this before changing the onboard PDM microphones, LED2, U10 MDIO, Titan programming flow, or interpreting the receipts from this campaign. This is a closeout inventory of what was learned through the 2026-09-13 session. It is not permission to collapse evidence boundaries or to inherit qualification from another image.

## What is true at closeout

- Titan UID `545433931bd25436593630352d068363` is running the LED2 diagnostic image with build ID `541209cf0ac855af45c2793967f64e1e3a31a4e901cb6b0cbb02556fd5a2589c` and HEX SHA-256 `c3bcea8a49c4461b51b7b8751bfcc5f6422430c65564c9a6381267550a0c55f7`.
- U10 answered repeated Clause-22 reads at schematic address 1 with ID1 `0x001C`, ID2 `0xC916`, combined identity `0x001CC916`, no I/O error and no page uncertainty.
- Firmware read back the owned BMCR/LCR fields and reported the complete `OFF, GREEN, OFF, YELLOW, OFF, BOTH, OFF` sequence.
- Captain directly observed that the LEDs worked. That is operator-reported optical evidence tied to this build and run, not an instrumented electrical or photometric capture.
- RGB LED3 remains the independent primary status lamp. LED2 failure must never take LED3 down.
- The onboard microphone image is not resident. Its earlier dual 16 kHz diagnostic capture remains valid only for its exact build and receipt.
- The G4-qualified image is not resident. Neither the LED2 image nor the microphone image inherits G4 qualification.
- LED2 is proved as a controllable two-die lamp on this Titan/build. Production application-status admission and concurrent workload regression remain separate work.

## Authority and ownership

| Item | Authority |
|---|---|
| Editable Titan firmware | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware` |
| Branch / base commit | `lane/k1-ra8p1-002` / `c2d4dcad1334b5b13c4a86546d2528075ce6366e` plus unpublished dirty work |
| Titan reusable knowledge | `/Users/spectrasynq/Workspace_Management/Software/agent-skills/packages/ra8p1-titan-engineering` |
| Immutable campaign evidence | `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/` |
| Research/support repository | `/Users/spectrasynq/SpectraSynq-EdgeAI-Lab`; not the production firmware edit location |

Do not create a sibling worktree, restore the checkout to an older review, stage unrelated files, or commit unless Captain asks. A staged build tree is a snapshot, not an edit authority. Preserve the unpublished microphone, palette, GPT, strip and LED work around this slice.

## Operator protocol: `Rearm` means execute now

This protocol is load-bearing because it failed twice in the wider session and was then repeated once more by the closing agent.

### Before telling Captain the image is ready

1. Finish source review and the required host tests.
2. Cross-build the exact target configuration.
3. Bind source hashes, ELF, HEX, build ID, cache state and compiler output in a receipt.
4. Choose a new programme receipt directory that does not exist.
5. Prepare the exact `programme_scalar.py` command in advance. Do not make Captain wait while the agent searches for it after `Rearm`.

### State machine

| Event | Required agent action | Forbidden action |
|---|---|---|
| Captain says `Rearm` | Start the already-prepared programmer waiter first; immediately reply exactly `WAITING` | USB census, diagnosis, rebuild, help lookup, source search, status explanation |
| Waiter reports `ROM_SEEN` / `ROM_IDENTIFIED_WRITING` | Report the observed stage promptly in chat: bootloader seen, then identified/writing | Leaving the only progress in a hidden log or asking for another button action |
| Waiter reports `WRITE_VERIFIED` (legacy `PROGRAMME_VERIFY_PASS`) and the operator has not transitioned | Say once: `Release USER and BOOT, then RESET.` | Extra status, JSON, ship path or a second copy of the instruction |
| Captain has already released/reset, or the application is responsive | Do not repeat the instruction; run the bound target scorer | Arguing about sequence or asking Captain to repeat a completed action |
| Programming fails | Preserve the failed receipt; state the exact failure and next discriminating action | Reuse the output directory, call it a pass, or start an unrelated investigation |

The closing agent violated the first row: after Captain said `Rearm`, it searched tool metadata, searched old artefacts and opened the programmer source before starting the waiter. This repeated the exact scar in the incoming handover. It then emitted a release/reset instruction after Captain had already acted. The prevention is not another reminder at handoff time; it is precomputing the command, putting this protocol in root `AGENTS.md`, and testing that both remain discoverable.

### Proven programmer command shape

Use absolute, already-verified paths and a fresh output directory:

```sh
python3 scripts/programme_scalar.py \
  --build /absolute/final-build-directory \
  --output /absolute/new-programme-receipt-directory \
  --wait-seconds 180 \
  --execute
```

The successful command in this campaign used `led2-mdio-diagnostic-preflight-02/pre-silicon` as the build and `led2-mdio-diagnostic-prog-01` as the fresh output. Do not reuse those paths for another write.

## Evidence levels: never merge them

| Level | What it proves | What it does not prove |
|---|---|---|
| Source review | The named source has a particular implementation and hash | Compilation, residence or hardware behaviour |
| Host transport test | Production functions behave under the edge-driven GPIO/PHY model | Live pad routing, electrical levels or a responding PHY |
| Target cross-build | The actual ARM compiler accepted the staged sources/configuration | That the HEX is resident or starts |
| Programme verify | Exact image bytes were written and verified on the identified UID | Application startup or peripheral behaviour |
| Application identity | The running image reports matching UID/build/source | That a particular peripheral observation is correct |
| Firmware register readback | The same firmware transport read back the requested owned bits | Independent analyser readback or physical light |
| Operator optical observation | Captain saw the physical LED sequence work | Waveform margins, photometry or future-build behaviour |
| Production admission | Status integration and workload regression pass in the admitted application image | Qualification of unrelated lanes |

Never infer the flashed image from the checkout, the checkout from the port, or current hardware state from an old receipt. Bind UID, build ID, source identity and HEX hash every time.

## LED2 technical canon

### Product boundary

LED2 is a two-die lamp connected to U10's LED outputs. Firmware controls U10 using management-register transactions. The LED2 lane does not need or authorise an Ethernet application, IP stack or network service.

### Physical route

For Titan Mini HW:V1.0:

- MDC: `PC11`, ball H5.
- MDIO: `PC12`, ball G5.
- PHY reset: `PA07`, ball F3.
- Schematic PHY address: 1.
- U10: RTL8211F-CG.

The public BSP aliases `ETHERNET_MDC=P415`, `ETHERNET_MDIO=P414` and `ETHERNET_RST=P708` are wrong for this schematic route. The original experiment used those aliases and could only read `0xFFFF`. Do not reopen this as a naming preference: the schematic nets and the successful address-1 identity transaction resolve the working route for this board/build.

`R_IOPORT_PinWrite` success proves a latch update request, not physical continuity. PFS, direction and PIDR still matter. The repaired diagnostic checks MDIO/MDC ownership per frame, reset state per probe attempt, raw PFS values, initial pin levels and DWT progress.

### Clause-22 receive phase

The production receive helper raises MDC, waits 2,000 CPU cycles, samples MDIO, then lowers MDC. It therefore samples late after the rising edge. For this exact helper:

1. Release MDIO and read an unclocked pre-ACK diagnostic level. Either level is allowed.
2. First clocked receive call is ACK and must be zero.
3. Next 16 clocked calls are D15 through D0.
4. One further released clock completes the read/idle phase.
5. On absent ACK, drain 32 released clocks while retaining failure.
6. Finish with MDIO released and MDC low; publish data only after final I/O checks.

The old implementation made two clocked turnaround reads. The second consumed D15, shifted `0x001C` into `0x0039`, and rejected `0xC916` because its leading one was treated as absent ACK. The old names `taz` and `ta0` made the interpretation look plausible while hiding the phase error.

This rule is helper-specific. Do not generalise it into deleting a turnaround clock from unrelated MDIO implementations; audit edges and sample phase.

### Why the old tests passed bad firmware

The old test checked source strings and compiled with `K1_LED2_PHY_STUB`. That branch bypassed the production GPIO transport. It could prove masks and state logic while the wire transaction remained wrong. The replacement `scripts/test_led2_mdio_wire.py` extracts the production transport functions and drives an edge-modelled PHY through GPIO/time seams.

The final suite covers 237 cases, including leading-one data, ACK absence and drain, recovery, write framing, every one-shot pin-write fault, release faults, read faults and MDC ownership loss. The backed-up original and five independent framing/release/drain mutations fail.

### Register and state repairs

- `paged_op()` must restore the original page after any operation following page selection. Preserve the first operation error and separately expose restoration failure/page uncertainty.
- Standard BMCR/BMSR access must first establish page 0.
- Preserve full register originals and change only owned bits. The BMCR owned mask includes reset so the reset bit is deliberately cleared rather than accidentally preserved.
- `apply_channels()` must read LCR back and compare the owned bits before marking a request applied.
- Observe BMCR, BMSR twice and PHYSR; propagate read errors. A partial/stale configuration cannot become READY.
- A mode timeout or apply failure drops capability/readback state and enters a bounded retry rather than hammering the bus.
- Releasing the Ethernet module stop and requesting the P309/A12 TXC route does not prove an RGMII TX clock. Keep `txc_state=unverified`. Identity and proven LED control do not create a general clock claim.
- Keep all-address scanning, reset sequencing and register traces out of the 7.5 ms production audio hop. They belong to standalone bring-up.

Candidate LCR values proved by the final firmware-readback plus operator optical run are:

| State | LCR candidate | Owned mask |
|---|---:|---:|
| OFF | `0x2100` | `0x6F60` |
| GREEN | `0x2040` | `0x6F60` |
| YELLOW | `0x0900` | `0x6F60` |
| BOTH | `0x0840` | `0x6F60` |

Do not replace these with guessed force-LED values. A future register change must repeat readback and optical association.

### Diagnostics and runner

- Opcode 20, subcommand 9 returns `L2T1` plus up to 96 fixed 20-byte records.
- Each record binds sequence number, attempt, reset-release age, address, register, ACK, I/O error, value and value validity.
- The trace is fixed RAM. Do not add synchronous formatted output inside a timed transaction.
- Readiness requires coherent repeated identity at address 1, family match `(id & 0xFFFFFFF0) == 0x001CC910`, valid owned-bit readback, page known, DWT running at 1 GHz and expected pin diagnostics.
- An ACK bitmap bit is not identity. A different responding address is strap-discrepancy evidence, not permission to rewrite the board contract.
- Evaluate identity independently of mode. Correct identity plus mode failure moves the investigation to configuration/clock state; it does not reopen MDIO framing without contradictory trace evidence.

### Failure interpretation if the problem returns

| Observation | Next discriminating action |
|---|---|
| GPIO API/PFS/PIDR disagree | Repair ownership/configuration and explain the raw fields |
| ACK present, ID shifted/wrong | Decode the exact captured frame and raw words |
| Stable `0x001CC916`, mode not ready | Investigate mode and clock configuration separately |
| All addresses unacknowledged after valid transaction/startup interval | Measure MDC/MDIO at the PHY side, reset timing, supply rails, reference clock and continuity |
| Firmware sequence/readback succeeds but light does not | Investigate U10 LED outputs, LCR association and LED2 hardware; do not blame MDIO identity |

The earlier all-`0xFFFF` scan was not proof that U10 was dead. It combined the wrong pins with a deterministic receiver defect. A high MCU read cannot establish PHY-side continuity.

## Onboard microphone technical canon

### Settled facts

- Capsules are LinkMems `LMD2718T261-OA1`, U13 and U14. They are not IM69D130.
- Shared PDM data is P502/PDMDAT2; clock is P812/PDMCLK2.
- Expected logical map is U14 LOW to channel 2 RISE as programme, and U13 HIGH to channel 0 FALL as measurement. Acoustic identity remains unproved.
- Use `k1_pdm_fifo16_extract`; the old raw `<< 1` extraction is removed.
- The live DMA ISR is `k1_pdm_target_dmac_isr`, not stock FSP `pdm_rxi_dmac_isr`.
- Publish the completed slot, then rearm only into a slot already reserved as FILLING. This prevents DMA from overwriting a published buffer.
- After both sequential `R_PDM_Start` calls, retrigger both channels through PDSTRTR to reduce pairing skew.
- GPIO strip emission can remain compiled, but its timed hop and opcodes 11/13/14 must not run while PDM owns the timing path.

### What the microphone run proved

`pdm-onboard-lmd2718-build-03`, HEX `4000461166200bdfda4cc7fcf8ba9010fdb5ddd79af9ce7ac6d7493b5c7b213d`, ran on the identified Titan. Over three seconds it produced 435 additional paired slots, distinct channel hashes, maximum pair skew 62 microseconds, zero overflow/drop recovery and zero rearm denial.

### What it did not prove

- Which physical capsule is U13 versus U14 acoustically.
- Safe gain/filtering: both 16 kHz lanes reached the int16 rails.
- Production audio: 16 kHz is diagnostic only.
- 40 kHz capture and measured-rate ASRC into the unchanged 24 kHz/180 AP contract.
- Live AP operation from the microphones.
- Any G4 qualification.

Never feed the 16 kHz diagnostic stream directly into the production AP or describe it as 24 kHz audio.

## Complete failure ledger

| Failure | Root cause | Resolution | Permanent prevention |
|---|---|---|---|
| Wrong PHY pins used | Public BSP alias names were trusted over the HW V1.0 schematic | Use PC11/PC12/PA07 | Schematic fact in Titan knowledge, regression and this canon |
| Every address returned `0xFFFF` | Wrong route plus receive framing defect; observation was overinterpreted | Correct route and framing; live identity now succeeds at address 1 | Do not diagnose PHY death from MCU all-high alone |
| ID words shifted/rejected | Two clocked turnaround reads consumed D15 | One clocked ACK, 16 data, one idle | Production-function edge model and leading-one vectors |
| Bad transport passed host tests | Stub and source-string assertions bypassed production GPIO code | Extract and compile production transport with seams | 237-case wire suite plus negative mutations |
| MDIO cleanup could hide damage | Early exits skipped release/drain or overwrote earlier errors | Bounded release retry, drain and error preservation | Fault injection across read/write/ownership failures |
| PHY page could be left changed | `paged_op()` returned before restoration | Single cleanup route; page uncertainty retained | Operation/restoration fault matrix |
| Requested channels were called applied | No LCR readback | Compare owned bits before `applied_valid` | Readback mismatch tests and live sequence receipt |
| Stale mode could become READY | `observe_mode()` errors and partial initialisation were ignored | Propagate errors, clear readiness, bounded retry | Target scorer rejects stale/missing fields |
| TXC was claimed from a variable | Module-start plus `txc_enabled=1` did not prove clock routing | Report requested but unverified | Keep TXC separate from identity and LED proof |
| Target diagnostics were ambiguous | Latest aggregate fields lacked register/attempt context | Fixed RAM `L2T1` trace | Image/boot-bound records before and after sequence |
| Diagnostic work risked audio deadlines | All-address scans and formatted dumps can take milliseconds | Keep startup scan/trace outside the hop | Production admission requires bounded update-cost regression |
| Microphone buffers risked overwrite | Rearm did not reserve the next FILLING slot first | Custom ISR with reserve-before-rearm | Rearm/fifo/capture host tests plus silicon counters |
| PDM channels were loosely aligned | Sequential starts lacked a common retrigger | PDSTRTR retrigger after both starts | Pair-skew receipt |
| Microphone samples clipped | Gain/filter path remains open | No false production admission | Rails are an explicit open gate |
| A diagnostic image was treated as inherited qualification | Flashed-image boundaries were blurred | Bind each claim to build/HEX/UID | No mic, LED2 or G4 cross-inheritance |
| `Rearm` did not start the waiter immediately | Agent tried to reacquire context after the operator command | Programme succeeded only after avoidable delay | Prepare exact command before announcing readiness; root AGENTS protocol |
| Completed button action was repeated | Agent relayed stale state instead of accepting current operator/device state | Proceeded directly to target validation | One finger instruction only; never repeat an acknowledged action |
| Technical status was reported at the wrong time | Agent prioritised explanation over operator timing | Restrict live messages to material gates | `WAITING`, one necessary finger instruction, then final result |

## Final receipts and current boundaries

| Gate | Receipt | Result |
|---|---|---|
| Repaired-source audit | `led2-mdio-source-recovery-01/` | Deterministic receive defect isolated; first repair cross-built |
| Final preflight | `led2-mdio-diagnostic-preflight-02/receipt.json` | Host tests, negative controls, source binding and ARM object inspection pass |
| Programming | `led2-mdio-diagnostic-prog-01/receipt.json` | UID-gated write and verify pass |
| Live control plane | `led2-mdio-diagnostic-run-01/receipt.json` | ID `0x001CC916`, address 1, mode/readback ready and complete firmware sequence pass |
| Optical | `led2-mdio-session-canon-01/captain-optical-observation.json` | Captain reported the LEDs worked; no instrument capture |
| Microphones | `exp-e-onboard-mic-e1e2.json` and `pdm-onboard-lmd2718-run-02/` | Dual 16 kHz diagnostic capture pass; not resident or production admitted |

The live runner reports TXC as requested but unverified. Its register evidence is firmware readback through the repaired path, not an independent analyser. Those limitations stay visible even though identity and emitted LED behaviour passed.

## Remaining ship path

### LED2

1. Map bounded application status events onto LED2 while keeping RGB LED3 primary and independent.
2. Measure real status-update cost and run the admitted concurrent workload regression.
3. Repeat build/UID/source/readback/optical receipts for that application image.

Do not rerun the standalone all-address scan in the production audio hop.

### Microphones

1. Reflash the named microphone image only when explicitly entering that lane.
2. Acoustically identify U13/U14.
3. Correct gain/filtering so neither lane reaches the int16 rails.
4. Implement 40 kHz capture and measured-rate ASRC into unchanged 24 kHz/180.
5. Prove live AP operation with receipts.

### G4

Restore the named G4 image only for work requiring its qualification. Do not rerun or regenerate frozen G4 checksums merely because another image was flashed.

## Required regression commands

Run after changing the LED2 transport, status integration, runner or this canon:

```sh
python3 scripts/test_led2_phy.py
python3 scripts/test_led2_phy_target.py
python3 scripts/test_status_led.py
python3 scripts/test_titan_session_canon.py
```

The canonical Titan skill must also pass:

```sh
python3 -m pytest -q tests/test_platform_memory.py
python3 skills/ra8p1-titan-engineering/scripts/platform_memory.py --topic led2 --check
```

## One-sentence rule

Prepare before `Rearm`; bind every claim to its image and evidence level; trust the schematic and measured transaction over friendly alias names; never promote host, firmware-readback or operator evidence beyond what each actually proves.
