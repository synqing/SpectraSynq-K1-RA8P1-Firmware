# Titan Emotiscope — Dependable Instrument v1

**THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.**

Claude Code execution brief · Captain assignment, 30 September 2026 · run end-to-end, then report.

## 0. What this is

This is a new, deliberately narrow assignment with five deliverables:

1. A tagged reference checkpoint.
2. Supervised host processes.
3. A dashboard that explains why a control isn't acting, and persistent identity for Faders 1 and 4.
4. One honest test command.
5. One precision A/B experiment on the real LEDs.

It does **not** replace or shrink `GROK-SMC-CONTROLS-EXECUTION-20260929.md`. Items from that brief that aren't listed here stay open, not cancelled. The anti-compression clause in `AGENTS.md` does not apply to this separate assignment.

Where `AGENTS.md` contradicts itself, the dated 29 September blocks win:

- emission on at boot
- default output 100/255 (the older 24/255 lines are superseded)
- one continuous 160-pixel surface, split 80+80 at pixel 80
- probe power off

## 1. Roots

| Prefix | Path |
|---|---|
| F — Titan firmware worktree | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001` (branch `feat/titan-emotiscope-port-001`) |
| S — SMC-Mixer host worktree | `/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001` (branch `feat/smc-titan-control-001`) |
| D — donor, read-only | `/Users/spectrasynq/Workspace_Management/Software/Emotiscope.HIL/v1.1_build` @ `7f86939bd7c16c4a6dd5789813e9d551246ed134` |
| E — lane evidence | `F/docs/emotiscope-port-001/evidence/` — put new evidence in fresh dirs `instrument-v1-<item>-01/` |

Read before starting:

- `F/AGENTS.md`
- `F/docs/emotiscope-port-001/CURRENT-STATE.md` (top section)
- `E/captain-broker-recovery-20260930-01/RECOVERY.md`
- `E/captain-host-repair-20260929-01/CLOSEOUT.md`
- `S/host/titan_control/EMOTISCOPE-OPERATOR-20260929.md`

## 2. Current state

This was verified by reading the files on 30 September. Recheck it live before acting.

**Resident image:**

- build `393527e4ff81eb5c4fe54e8be3bf874bc37dce9fd89a2d9fec3d4b3e7ac8c4cb`
- schema `5f3c9fa6…6f60`
- HEX `73ad9be5…0071`
- ELF `ed997e83…849f`
- UID `545433931bd25436593630352d068363`
- build receipt: `E/arm-smc-controls-001/receipt.json`, whose `sources` lists a SHA-256 for every compiled file

**Broker** (the sole USB-serial owner of Titan):

- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 F/tools/serial-studio/titan_broker.py run --evidence /tmp/titan-emotiscope-control --live-control --continuous-output`
- It runs detached (PID 32626 at last record) with **no supervisor**.
- Its socket and approval files (`accepted-candidate.json` / `accepted-checkpoint.json`) live in `/tmp/titan-emotiscope-control`. macOS empties `/tmp` at reboot.
- The persistent copy of the admission is `E/smc-controls-001/accepted-candidate.json`.
- `titan_control_server.py` also checks `TITAN_LIVE_CONTROL_ADMITTED=1`.

**Mixer controller:**

- The exact command is in the operator card: `S/host/titan_control/.venv/bin/python -m smc_titan_control live --strategy emotiscope …`, using the Bluetooth endpoint, dashboard on 8776 and status on 8767.
- It was PID 79233 at last record.
- It survives broker loss and reattaches.

**Faders 1 and 4 stay blocked because proof is thrown away.** `LabelledCapture` in `S/host/titan_control/smc_titan_control/emotiscope_session.py` keeps proofs per transport epoch and "never auto-loads" them. Every BLE (Bluetooth Low Energy) reconnect or controller restart therefore re-blocks the faders. `finish()` also admits the proof in memory before its result file is written.

**The dashboard shows pickup only as "picked up / cross current value".** It shows no position, no target and no direction. Broker loss and mixer loss both land in the same `transport_error`/`last_error` text. `LAUNCH` still says USB/8766.

**The host tests give a false result.** `S/.venv` (Python 3.13) has no pytest, so `run_tests.py` reports 2 import errors (`test_assignments.py`, `test_dashboard_browser.py`). The "155 passed" figure came from a different interpreter (Python 3.14).

**Output path (verified in source):**

1. Donor float `leds` (post-gamma, 0–1) → `quantize_color_error()` → RGB8 with temporal dither (`emotiscope_donor/led_driver.h`).
2. `emotiscopeWirePixel()` does nearest 128→160 mapping and multiplies by 257 (`emotiscope_output.h`).
3. `scale16` applies the output brightness once, in `PaletteRuntime::packBenchGrb48Lane` (`palette_runtime.cpp`).
4. The result is packed as GRB48 (16 bits per colour on the wire).

## 3. Standing rules

- **One owner each.** One USB-serial (CDC) owner, which is the broker. One MIDI owner, which is the controller. Never start a second of either.
- **Host events never change the show.** A host start, stop, crash or reconnect never flashes, resets, restores defaults, turns emission off or changes brightness. On attach, read the board first and send nothing.
- **Git.** Narrow commits on the lane branches only. No merge to main. Follow `docs/process/` git and housekeeping doctrine. Don't use `git add -A` unless every path is proven.
- **Docs.** Update only the top of `CURRENT-STATE.md` and the operator card. Write no other new prose docs.
- **Audio.** Use Sonic Samba and Carte Blanche only. Dopex stays held out. Song-0 is banned.
- **Captain HOLD overrides everything.**
- **If a check fails, keep the current image and controller running.** Report the exact blocker and carry on with independent items.

## 4. Work items, in order

### 4.1 Reference checkpoint (tag, no rebuild)

1. **F:** SHA-256 every path in the receipt's `sources` against the worktree.
   - All match: commit the owned uncommitted source on the lane branch and tag `titan-emotiscope-ref-r1`. For evidence binaries (ELF/HEX), follow the lane's existing convention: check what earlier commits tracked.
   - Any mismatch: don't tag. List the mismatches and explain each one, for example work made after the flash.
2. **S:** check the files in `E/captain-host-repair-20260929-01/source-after.json`. Commit the `host/titan_control` source (not `.venv`, `__pycache__`, `*.egg-info` or `reports/`) and tag `titan-mixer-ref-r1`.
3. First confirm that no other agent is mid-edit in either worktree: check running agent processes and files changed very recently. If one is, skip this item and report.

**Done when:** both tags point at source that matches what is running, or a list of mismatches is reported.

### 4.2 Supervise broker and controller with launchd (macOS's service manager)

**Agents.** Create two per-user LaunchAgents in `~/Library/LaunchAgents`: `com.spectrasynq.titan-broker` and `com.spectrasynq.titan-mixer`. Give each:

- `RunAtLoad` true
- `KeepAlive` = `{SuccessfulExit: false}`, so it restarts after a crash and stays down after a clean exit
- `ThrottleInterval` 10
- `StandardOutPath`/`StandardErrorPath` set to `~/Library/Logs/SpectraSynq/<label>.log`
- absolute interpreter and script paths, and a `WorkingDirectory`
- `EnvironmentVariables` copied from the live processes (`ps eww <pid>`)

**Broker wrapper.** Add a small script that creates `/tmp/titan-emotiscope-control` and copies the retained admission file byte-for-byte, checking its SHA-256, then `exec`s the broker. Never generate or edit admission content.

**Handover:**

1. Stop the controller with SIGINT (its clean path).
2. Stop the broker with `titan_broker.py stop`.
3. Bootstrap both agents.
4. Titan's autonomous show continues throughout. Confirm this from readback.

**Operator commands.** Record these three in the operator card:

- start: `launchctl bootstrap gui/$(id -u) <plist>`
- stop: `launchctl bootout gui/$(id -u)/<label>`
- restart: `launchctl kickstart -k gui/$(id -u)/<label>`

**Programming handoffs must use `bootout` first.** `titan_broker.py stop` falls back to SIGKILL after 8 s, and launchd would treat that as a crash and restart the broker.

**Expected behaviour.** If Titan is unplugged, the broker restarting every 10 s until it returns is intended. If a macOS privacy prompt (TCC) blocks Bluetooth or USB for the agent-launched process, report it for the Captain to approve. Don't work around it.

**Crash test.** Run `launchctl kill SIGKILL gui/$(id -u)/com.spectrasynq.titan-broker` and check:

- the broker returns within the throttle interval
- the controller reports the loss, then reattaches with a read-first attach and no write
- mode, output and emission are unchanged in readback
- frames advance throughout

Then confirm that `bootout` keeps it down and `bootstrap` restores it.

**Done when:** both processes run under launchd, the crash test evidence is in E, and the reboot-safe admission copy is verified by hash.

### 4.3 Dashboard that says why, and persistent fader identity

**Dashboard.** Extend the existing `emotiscope_dashboard.py` and `status()`. No new site and no framework. Separate these states, each with plain text:

| Situation | Show |
|---|---|
| Broker socket down | **Titan connection unavailable** |
| MIDI endpoint or BLE epoch missing | **No mixer input** |
| Absolute control not picked up | A bar with a **position** marker (last raw CC) and a **target** marker (confirmed value in fader space, with saturation squared), plus "Move up to 56%", "Move down to 39%" or "Active" |
| Identity not qualified | **Blocked — needs labelled capture**, with its capture button |
| Control has no effect in this mode | From the existing applicability data, e.g. "Speed affects Analog only" or "Reverse needs colour range above 0" |
| Write in flight | **Pending**, then accepted/rendered, or the explicit error |

**Identity persistence.** Persist a passing labelled proof and reload it only when all of these match:

- endpoint name
- driver
- unique ID
- BLE peripheral ID
- SHA-256 of `profiles/emotiscope-v1.json`

Any change invalidates the proof. Pickup and held-button state still reset on every epoch, as they do now.

**Fixes:**

- Reorder `finish()`: write the raw `.jsonl` and the result `.json`, fsync both, and only then admit the proof. A failed write admits nothing.
- Correct the stale `LAUNCH` string.

**Tests:**

- a proof reloads when identity matches
- a proof is rejected when any single key changes
- a failed persistence admits nothing
- status output for each of the six situations above

**Done when:** tests pass, the controller is restarted via `kickstart`, and the dashboard shows the new states live.

### 4.4 One honest test command

1. Add a `test` optional extra with `pytest==8.4.2` to `S/host/titan_control/pyproject.toml` and install it into `S/.venv`.
2. Add `F/scripts/check_instrument.sh` with `set -euo pipefail`. It prints the interpreter paths and both worktree HEADs plus dirty flags, then runs:
   - `S/.venv` → `S/host/titan_control/run_tests.py`
   - `python3.14 -m pytest F/tools/serial-studio/tests`
   - `F/scripts/test_live_protocol.py`
   - `F/scripts/test_emotiscope_controls.py`
   - `F/scripts/test_emotiscope_output.py`
3. Its exit status must be the real result: no trailing `echo` and no `|| true`.
4. Prove it once: inject one failing assertion, show a non-zero exit, then remove the assertion.

**Done when:** it exits 0 on the clean tree and non-zero on the injected failure.

### 4.5 Precision A/B test (the only firmware change)

**Candidate.** Take the same validated float `leds` that feed `quantize_color_error()` and convert them as follows:

- round `v × 65535` to 16 bits
- use the same nearest 128→160 mapping
- no ×257
- the same single `scale16` output gain
- no second gamma and no extra gain
- no 8-bit dither in the candidate, so dithering shows as not applicable there

Keep the reference path computing every frame, so switching back is seamless.

**Selection.** Add one masked-PATCH field, `output_precision`:

- 0 = reference RGB8, 1 = candidate 16-bit
- boot default 0
- excluded from scenes and defaults-restore, like emission
- applied by the real-time owner at a frame boundary
- returned in GET

Update the schema, codecs and catalogue together. The new schema hash replaces the old one, and old-schema writes still reject.

**Host controls.** Add a dashboard toggle. The Record button (ch0 CC54) toggles from confirmed state on a fresh press.

**Tests.** Use the existing output harness:

- With 0, wire bytes are identical to today's over the 1000-frame fixture. This proves the reference is untouched.
- With 1, each wire value equals the rounded, mapped, once-gained float; the 80+80 split and endpoints are unchanged.
- Report two numbers, not pass/fail: the mean wire-level difference between paths, and the count of distinct non-zero levels on a dim-fade fixture for each path.

**Deploy:**

1. Build with the existing `scripts/build_scalar.py` flags from the Grok brief §9, into `E/arm-precision-001`.
2. `launchctl bootout` the broker agent.
3. Programme through the `ra8p1-e2lite-router` with `scripts/programme_rfp_swd.py`:
   - probe `5AS079228B`, SWD 1 MHz, probe target power OFF
   - `-noquery -run`, range `02000000,02C9F01F`
   - exact UID and receipt hashes
4. Read back the full UID/build/schema/boot/config. Confirm Spectrum, 100/255, emission on and advancing frames.
5. Write the new admission through the existing process, with this build's identities and honest scope text. Point the wrapper at it, then bootstrap.

The Captain authorises this image under the standing SMC–Titan authority, so there is no approval pause. A failed check means no flash.

**Done when:** the new image runs with `output_precision` 0, behaves as before, and the toggle round-trips in readback.

## 5. One Captain session (≤ 15 minutes)

Cue the Captain **once**, only after 4.1–4.5 are done. Give him a numbered card and record raw MIDI → accepted/rendered/emitted IDs for each step:

1. **Labelled captures** on the dashboard: Fader1, Fader4, R5, R8, Record. Faders 1 and 4 should then arm.
2. **Sweep:** Faders 1–8, Knob1, M1–M7, S1–S5, Previous/Next, Play/Pause (finish with emission on). Save scene SQ1, change some controls, recall SQ1.
3. **Reconnect:** the Captain switches the mixer off and on. Pickup resets. Faders 1 and 4 **stay armed**. Nothing is written on reattach.
4. **A/B:** use Spectrum and then Bloom through a breakdown, at output 100/255 and again at a low output level. Flip Record between reference and candidate. Record the Captain's verdict word for word (better / same / worse). Don't change the boot default without his explicit word.

## 6. Handback (short, plain English)

- Resident build/HEX/schema, mode, output and emission.
- Each item from 4.1–4.5 and §5 marked PASS / FAIL / BLOCKED / NOT TESTED, with its evidence path.
- Tags and commits.
- The three `launchctl` commands.
- The A/B numbers and the Captain's verdict.
- Anything left open, with its owner.

## 7. Out of scope — do not do

- audio-input or render-timing audits
- music intelligence
- PRISM-Core extraction or shared adapters
- new effects or centre-origin motion
- AddressSanitizer additions
- long soak tests
- NPU (neural processing unit), Helium/MVE vector-extension or multicore work
- changing donor defaults
- USB-transport mixer testing
- cold power-cycle experiments
- new dashboards or preview sites
