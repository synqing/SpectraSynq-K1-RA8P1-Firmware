# Titan Mini LED2: MDIO recovery handover

> **Closed by later evidence, 2026-09-13.** The transport and downstream repairs were completed, exact build `541209cf...` was programmed to UID `545433931bd25436593630352d068363`, U10 returned `0x001CC916` at address 1, the complete firmware-readback sequence passed, and Captain reported the physical LEDs working. Read `TITAN-MIC-LED2-SESSION-CANON.md` and the `led2-mdio-diagnostic-*` receipts before acting. The historical open conclusions below remain useful failure context but are no longer current target status.

The job is to use LED2 as a second application status lamp. Its two dies are wired to U10's LED outputs, so the firmware must control U10 over MDIO. This does not require an Ethernet application, cable, IP stack, or network service. RGB LED3 remains the independent primary status lamp.

This handover follows an audit of the actual unpublished firmware on the Mac, not just the failure report. Narrow source repairs have been applied there. No serial device was opened and nothing was programmed during this review. The last reported resident image remains build-06; its identity has not been freshly queried by this review.

**Actual firmware checkout:** `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`

**Backups, source hashes, host logs and receipt:** `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/led2-mdio-source-recovery-01`

## What was actually wrong

The source has a deterministic MDIO framing error. Its receive helper raises MDC, waits 2,000 CPU cycles, samples MDIO and lowers MDC. With that late-sampling convention, its first receive call after the request captures ACK. The next captures data bit 15. The old code called these `ta_z` and `ta_zero`, then started a 16-bit read one bit too late.

The isolated production-function demonstration returns `0x0039` for a PHY word of `0x001C`, and rejects `0xC916` because its leading one is mistaken for a missing ACK. This is an implementation defect independently of whether the physical board has another fault.

The corresponding phase convention in [Linux v6.12's MDIO bit-bang driver](https://github.com/torvalds/linux/blob/v6.12/drivers/net/mdio/mdio-bitbang.c) is one ACK receive call, 16 data calls and one idle call. Linux reads just after falling MDC; Titan reads just before it. Both read after the same rising edge has settled. The [RTL8211F datasheet, Rev. 1.4, sections 7.11.2 and 10.6.1](https://xonstorage.z8.web.core.windows.net/pdf/realtek_rtl8211fcg_Lcs01_link.pdf) gives the relevant read timing. This is not a general instruction to delete a turnaround clock from every MDIO implementation: audit the helper's exact edges and sample phase.

| Corrected receiver operation | Meaning |
|---|---|
| Release MDIO; read its level without generating a clock | Optional diagnostic pre-ACK level; either value is allowed |
| First late clocked receive sample | ACK, which must be zero |
| Next 16 late clocked samples | D15 through D0 |
| One further clock with MDIO released | Idle/end of read |
| ACK absent | Drain 32 released clocks, retain the failure |

Build-06's `taz=1, ta0=1` is not a trustworthy decoding of the named turnaround positions. The captured high levels are real reported observations, but their protocol interpretation is defective. Fixing this does **not** establish that the board will respond: the first high sample can still reflect a separate startup, pin-control or electrical problem.

The old host test checked source strings and compiled with `K1_LED2_PHY_STUB`, which bypasses the production GPIO reader. It could not establish correct wire framing. The pending ACK-bitmap insertion also invalidated its literal source-string assertion.

## Repairs already applied

1. `platform/ra8p1/titan_led2_phy.c`: correct the late-sampling ACK/data sequence; explicitly enter output mode at each frame; finish successful reads/writes with MDIO released and MDC low; drain failed reads; retain I/O errors through cleanup; publish received data only after the final error check. A failed input-mode change gets one bounded release retry and no receive clocks.
2. The same source: retain the pending pin-readback/ACK-bitmap diagnostics; allow nominal 10 microseconds of settling before each initial pin-level check; use 200 ms after reset release for conservative diagnostic startup margin. The 20 ms reset assertion remains. Retries already leave reset deasserted; no reset-on-every-retry defect was found. Neither the 10 microseconds nor the 200 ms is asserted to be a silicon minimum.
3. `scripts/test_led2_mdio_wire.py`: compile the actual production transport functions with GPIO/time seams and an edge-driven PHY model. This is independent of the high-level stub branch.
4. `scripts/test_led2_phy.py`: invoke that wire test and retain the existing high-level state/mask tests. Remove the incorrect requirement for two clocked receive helpers before data.
5. `scripts/run_led2_phy_target.py`: observe readiness for five seconds by default; preserve timestamped observations and partial sequence samples; require coherent identity/address and readiness before restarting the sequence; retain strict UID/build/source checks. An I/O or restoration failure fails immediately. An unavailable identity/mode can be observed through several retry periods.
6. `scripts/test_led2_phy_target.py`: retain the original scorer negatives and add fake-time readiness, deadline, identity, I/O and partial-evidence tests.

The runner now accurately labels its channel evidence as firmware-reported. It does not claim independent register readback. Its host polling cannot prove how many firmware retries occurred; the receipt says `firmware_retry_count=NOT_OBSERVED`. An inherited blocking serial read can extend wall-clock duration beyond the readiness deadline; a late successful reply is recorded but cannot pass timely readiness.

**Host results on the Mac:** production transport 37 cases PASS; existing LED2 state tests PASS; target runner/scorer 10 tests PASS; existing status-LED tests PASS. Running the new wire suite against the backed-up original source fails, as required. Five separate earlier transport mutations were also rejected during the audit: extra ACK clock, omitted ACK clock, early sampling, missing release and missing failure drain.

**Target cross-build:** PASS, using build-06's scalar O3/unroll configuration with D-cache disabled and the same disabled optional lanes. The build includes the previously uncompiled pin-readback/status changes. Build ID: `b1e8e8bc36c36e2f04d9db1c7838517a6847bdfc6fad811ee3e8d68358afb3d9`. HEX SHA-256: `c17d483df18acc62721d74fd6252265994405af8c2032b9b14755e444df78bb3`. Receipt: `led2-mdio-source-recovery-01/pre-silicon/receipt.json`. This is a built artifact, not the image currently on Titan. No programming approval or hardware qualification is implied.

These tests do not validate the live DWT clock, electrical levels, pad routing, reset, 25 MHz reference, concurrent workloads or emitted light. Write-success framing is covered; exhaustive write-side GPIO-fault recovery is not.

## Instructions for the next CLI agent

1. **Start from the repaired tree.** Read this file and `source-manifest.json` in the evidence directory. Preserve surrounding unpublished changes. The five edited/new source and script paths are scoped in that manifest; do not restore the whole checkout or stage unrelated work. Re-run the three commands below if changing any of them.

   ```bash
   python3 scripts/test_led2_phy.py
   python3 scripts/test_led2_phy_target.py
   python3 scripts/test_status_led.py
   ```

2. **Finish the diagnostic preflight before requesting rearm.** Bind the final source hashes, target ELF/HEX and build receipt. Check the actual target compiler output, not only host C. Confirm the pending header/status-field changes are included in the build. Confirm DWT CYCCNT is running at the expected clock and that pin ownership remains GPIO throughout the transaction. Keep cache disabled and the existing scalar configuration. Do not reset or repurpose the application's DWT counter.

   The physical pin facts remain PC11/H5=MDC, PC12/G5=MDIO, PA07/F3=PHY reset. The public BSP aliases naming P415/P414/P708 are inappropriate for this schematic. FSP API success alone does not prove routing or continuity: `R_IOPORT_PinWrite` updates the latch and `R_IOPORT_PinRead` samples PIDR. Capture relevant PFS fields as well as the input level when diagnosing a mismatch. See the [FSP IOPORT implementation](https://github.com/renesas/fsp/blob/v6.5.0/ra/fsp/src/r_ioport/r_ioport.c).

3. **Make the next target observation discriminating.** Existing diagnostics retain the ACK bitmap and expected-address turnaround levels. Before calling a single failed result a hardware diagnosis, record which register/attempt the fields describe. Prefer a small fixed RAM trace or compact snapshot containing attempt number, reset-release age, address, register, ACK, I/O error and read value. Keep history tied to this image and boot. Avoid synchronous formatted dumps in a timed hop. The current all-address scan itself can occupy several milliseconds; it belongs in this standalone bring-up phase, not an admitted 7.5 ms production audio slot.

4. **Use one converged diagnostic image and one operator handoff.** Complete software review and tests first. The current compiled artifact, if present under `pre-silicon`, is compilation evidence for this repair; it does not waive the remaining diagnostic or register-control work. Do not ask for a button to try a source guess. Once the CLI agent has the final image ready, follow the existing programming/UID/build/source procedure and request USER/BOOT only when the programmer is actually armed. This review did not perform that step.

5. **Evaluate the identity independently of loopback readiness.** A valid identity requires repeated exact reads of ID1 and ID2 at the schematic address, with coherent combined identity and no I/O error. Expected ID1 is `0x001C`; the RTL8211F family match is `(id & 0xFFFFFFF0) == 0x001CC910`, with the revision nibble reported, not invented. A responding different address is diagnostic evidence and a strap discrepancy, not permission to silently change the board contract. An ACK bitmap bit alone is not identity.

6. **Interpret failure using the right boundary.**

   | Observation after a valid software transaction/startup interval | Next action |
   |---|---|
   | GPIO API/PFS/PIDR discrepancy | Repair or explain pin configuration, ownership and settling; API return success is insufficient |
   | ACK seen, wrong or shifted ID | Audit exact captured frame, address and data sampling; report raw words |
   | Stable correct ID, mode not ready | Management works; investigate the loopback/TX-clock configuration separately |
   | All addresses remain unacknowledged | Capture MDC/MDIO at the PHY side, with reset timing; then check PHY supply rails, 25 MHz reference and continuity |
   | Firmware reports valid channel sequence | Require actual register readback and optical observation before admission |

   The last all-high case is reason to measure, not proof that U10 is defective. MCU readback cannot establish PHY-side continuity. Keep reset high during repeated probes. Inspect the board's reset RC, MDIO pull-up, PHY supply path and reference clock against the schematic; do not hold reset low while diagnosing whether the PHY's own core supply has started. P107's PHY interrupt net can offer corroborating readiness evidence if its routing/default interrupt semantics are checked; it is not a substitute identity gate.

7. **Resolve the downstream source defects before claiming four-state control.** These were found during review and are not repaired by the transport patch:

   - `R_BSP_MODULE_START(FSP_IP_ETHER, 0)` only releases module stop. Assigning `txc_enabled=1` afterward does not configure or prove a valid RGMII TX clock. Implement the required clock/pin route from the actual board and PHY loopback requirements, or report clock state as unverified. Basic MDIO identity must not depend on networking or link-up.
   - `paged_op()` has early write-error exits that skip restoring the original page, and one read-error cleanup path discards a restoration error. Route every operation after page selection through restoration; preserve the first error and flag failed restoration/page uncertainty. Cover both operation failure and restoration failure with host fault injection.
   - `apply_channels()` currently marks the request applied after writes, without reading LCR back. Read back owned bits before claiming register application. A requested `g/y` pair is not physical readback.
   - Validate the actual BMCR/mode/status bits and handle errors from `observe_mode()`; it currently discards that return value. Establish a known standard-register page before reading/writing BMCR. Handle initialization failure and restoration explicitly so a partially configured device cannot become READY from stale flags.
   - Keep full-register originals and preserve unrelated/strap-dependent bits. The existing candidate LCR encodings remain OFF=`0x2100`, GREEN=`0x2040`, YELLOW=`0x0900`, BOTH=`0x0840` within owned mask `0x6F60`. These are an existing loopback-based control design, not independently proved light. Do not replace them with guessed force-LED register values.

8. **Close the gates in order.** Identity; configured/observed loopback mode; owned-bit readback for OFF/GREEN/OFF/YELLOW/OFF/BOTH/OFF; observed light associated with each state; bounded application-status integration; workload regression. Keep RGB LED3 working as the primary lamp during LED2 failure. Do not run the MDIO scan, reset sequence or register dump in the audio interrupt/hop path. Check recovery after restart and the cost of real status updates before admitting this backend to production.

This work does not alter the G4 frozen checksums or qualify a microphone, strip transmitter, NPU or complete product. LED2 remains unqualified on hardware until the new target evidence exists.
