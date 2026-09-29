# Grok execution brief — Emotiscope controls on Titan through SMC-Mixer
29 September 2026 • Implementation, physical verification and closeout

**THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.**

## 1. Assignment and finish line

Expose the running Emotiscope engine’s complete supported musical control surface through the existing M-Wave SMC-Mixer host. Implement the firmware contract, broker, mappings, state display and persistence needed to make those controls usable. Build and programme the exact checked image, exercise real mixer controls against Titan, and leave autonomous music running.

The user now reports that the Emotiscope firmware looks stable on Titan. Treat that as the current operator observation and preserve this image as the working baseline. This is a controls integration phase. Do not restart the visual redesign, recover the rejected Aurora/Bloom lane, or substitute legacy K1 AP/VP controls for donor controls.

Completion means:

- Eight native continuous controls, five native toggles and all seven musical modes are available from the physical SMC-Mixer.
- Titan output brightness is separately identified and controllable; explicit emission on/off remains available.
- The host shows what the board actually accepted and used, with pickup, applicability and connection state.
- Real hardware gestures reach the donor/adapter consumers without audio loss, rendering stalls, accidental resets or changed strip geometry.
- Exact source, build, programme and target evidence identify the installed result. The operator can immediately use the mixer and inspect the actual LEDs.

Execute through verification and closeout. Do not return another “implemented but not programmed” or “ready for permission” report when the relevant checks pass. A genuine failed prerequisite stops its dependent write; it does not justify abandoning unrelated implementation.

This brief was prepared from source and retained receipts. No target access, playback, build or programming was performed while preparing it. Fresh target identity remains a deployment prerequisite.

## 2. Authoritative locations and baseline

Use these exact roots. Paths prefixed `F/`, `S/` or `D/` below resolve against this table.

| Prefix | Absolute path |
| --- | --- |
| F — active Titan port | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001` |
| S — existing SMC host | `/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001` |
| D — read-only original donor | `/Users/spectrasynq/Workspace_Management/Software/Emotiscope.HIL/v1.1_build` |
| Parked effects worktree | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-effects-language-001` |

Titan branch: `feat/titan-emotiscope-port-001`; source/evidence checkpoint: `2ef4fe5`. Donor commit: `7f86939bd7c16c4a6dd5789813e9d551246ed134`. Recheck actual Git state before edits; do not reset to these identifiers over later work.

| Recorded resident identity | Value |
| --- | --- |
| Titan UID | `545433931bd25436593630352d068363` |
| Build | `72773a7461590edbdb53080670f6c61608fb973c889f7bdcc1b9a65da4b8319b` |
| HEX SHA-256 | `28acf53a54259ac6f8e8d0a2ef3e6fda7aa5d513859310f757de3ba2edd815eb` |
| ELF SHA-256 | `d1d446c6f4db432a0b1e4cd53f7df8fb9b437e2057ec663d2ed9be91b4693fb7` |
| Current schema SHA-256 | `c7b67f1eecbe836721c97331eef895813857191e273999c96b1f3fbf41bce39a` |
| Normal boot | Spectrum, mode 1; emission ON; hardware brightness 100/255 |

Read first:

1. `F/AGENTS.md`, especially its latest Emotiscope, brightness, geometry and continuous-music instructions.
2. `F/docs/emotiscope-port-001/CURRENT-STATE.md`, `EXECUTION.md`, `DONOR-SOURCE-MANIFEST.json` and `CLOCK-PORT-DELTA.json`.
3. `F/docs/emotiscope-port-001/evidence/arm-port-004/receipt.json`, `programme-port-004/receipt.json`, `target-summary-004.json`.
4. Applicable `AGENTS.md` files in the SMC tree and its current source, profiles, `host/titan_control/DASHBOARD.md` and `SURFACE-VIEWER.md`. Some older prose is stale; resolve it against the newer decision records and actual source below.
5. Before target work, the canonical Titan platform-memory and E2 Lite instructions required by `F/AGENTS.md`. The router is `/Users/spectrasynq/.cursor/skills/ra8p1-e2lite-router/SKILL.md`. Do not propagate its superseded 24/255 or historical wiring clauses.

Current source and CURRENT-STATE govern the completed port. For example, the original execution proposal said spatial interpolation; the installed adapter uses nearest-source-pixel expansion. Preserve the installed, tested adapter.

The existing SMC tree has substantial dirty and untracked implementation work. Capture its actual file hashes and patch before editing; a Git HEAD alone does not identify that host. Preserve assignment files, scenes, prior calibration evidence and unrelated work. Stage only owned paths.

## 3. Fixed behavior and scope

The active path must remain:

Titan microphone capture → direct measured-rate conversion to 12,800 Hz / 64 samples → donor Goertzel, chroma, VU and tempo → donor visual/output stages → donor gamma and temporal RGB8 dithering → native 128-to-160 output adapter → RGB8 expansion to GRB48 → one hardware brightness scale → DIN-A/B.

Keep these invariants:

| Item | Requirement |
| --- | --- |
| Audio period | 64 samples / 5 ms; preserve the donor AP and render-side novelty/phase updates |
| Render schedule | Existing declared 100 Hz; do not retime it as a controls shortcut |
| Geometry | One continuous logical 160-pixel surface; centre between 79/80 |
| DIN-A | Logical 0–79, P601 / GPT6 / DMAC0, 80 pixels / 480 bytes |
| DIN-B | Logical 80–159, P604 / GPT8 / DMAC3, 80 pixels / 480 bytes |
| Microphone | DMA1/2 unchanged |
| Output | Preserve donor processing order; no extra gamma, palette, gain or independent scene per DIN |
| Defaults | Autonomous emission ON; hardware brightness at least 100/255, currently exactly 100 |
| Control disconnect | Does not disarm, reset, restore defaults or stop analysis/rendering |
| Engine lifetime | Init once at boot; controls and mode changes preserve AP, colour, visual and dither histories except native documented transitions |
| Review audio | Trance/EDM only. Song-0 remains banned. Use Sonic Samba and Carte Blanche; Dopex remains held out |
| Review medium | Actual Titan LEDs and physical mixer. No HTML film or rendered lamp approximation as appearance proof |

Explicit operator dimming/off remains allowed; the minimum is the normal/default brightness, not a prohibition on deliberate dimming. A host crash, timeout, MIDI disconnect or diagnostic completion is never an operator off command. Programming necessarily interrupts the application; restore autonomous operation on application return and report that interruption honestly.

Allowed changes: the active Titan port’s control/schema/runtime integration, existing SMC host, relevant tests, build inputs and documentation. Donor original, vendor BSP, DualMCU, K1.Sliders and K1-System-R3 stay untouched. Preserve donor attribution. Do not copy donor network credentials into logs or new documents.

No AP retuning, alternative effects, invented frequency EQ, input-gain redesign, new LED driver, MCU partitioning, NPU/MVE work or storage subsystem is needed for this task. Fix a demonstrated control-related numerical/ownership defect narrowly and record its delta.

The parked AP/VP/CTL/MOD/IVV assignments remain parked/open as recorded. This integration does not close them by implication.

## 4. Complete control contract

The portable declaration is `F/platform/ra8p1/emotiscope_port.h`. Defaults and validation are in `emotiscope_port.cpp`. Copied native consumers are under `F/platform/ra8p1/emotiscope_donor/`.

Use stable donor-specific field names. The wire/API config values are the native raw values below; physical UI conversions are separate metadata.

| Native field | Type / accepted input | Default | Meaning and caveat |
| --- | --- | --- | --- |
| brightness | finite float 0–1 | 1.00 | Native artistic level: scale is 0.3 + 0.7 × value. Zero is not blackout |
| softness | finite float 0–1 | 0.25 | Native image low-pass response; test on changes, not only a settled frame |
| color | finite float 0–1 for explicit input | 0.33 | Hue; auto-color owns its ongoing evolution when enabled |
| warmth | finite float 0–1 | 0 | Native incandescent colour treatment |
| color_range | finite float 0–1 | 0 | Native hue spread |
| speed | finite float 0–1 | 0.50 | Effective in Analog; inapplicable elsewhere except the retained, ineffective Bloom control |
| saturation | finite raw float 0–1 | 0.75 | Native colour saturation; original UI input is square-root transformed |
| background | finite float 0–1 | 0.25 | Native additive background; not physical output gain |
| current_mode | integer 0–6 for this musical profile | 1 | Analog, Spectrum, Octave, Metronome, Spectronome, Hype, Bloom |
| mirror_mode | boolean | true | Native scene/background geometry; never transfer length or a second DIN scene |
| screensaver | boolean | true | Native silence-driven visual blend; does not switch off the transmitter |
| temporal_dithering | boolean | true | Native fractional RGB8 error accumulation |
| auto_color_cycle | boolean | false | Native novelty-driven hue motion |
| reverse_color_range | boolean | false | Native hue/range expression; ineffective when range is zero |

Also expose **hardware_output_brightness** as integer 0–255, default 100, applied once in the existing final output adapter, and explicit **emission_enabled**. Display “Donor brightness” and “Output brightness /255” distinctly. Do not alias the two or multiply either twice.

Support native musical mode IDs exactly:

| ID | Mode | Principal musical driver |
| --- | --- | --- |
| 0 | Analog | Smoothed VU |
| 1 | Spectrum | Smoothed spectrum |
| 2 | Octave | Chromagram |
| 3 | Metronome | Native tempo-bank magnitudes and phases |
| 4 | Spectronome | Spectrum and native tempo |
| 5 | Hype | Native tempo/beat-confidence behavior |
| 6 | Bloom | Chromagram and native image history |

The portable setter also accepts Neutral 7, which is deliberately non-musical. The current deployed profile exposes only 0–6. Keep the normal physical mode cycle and advertised musical modes at 0–6. Inventory Neutral as an excluded utility, not an eighth musical effect. Self Test 8 is rejected by the setter and must not become selectable merely because a name function returns its name.

“All controls” means all fourteen supported musical config fields above and the real Titan output controls. It does not mean inventing operative switches for excluded ESP networking, OTA, reboot, NVS or debug exporters. Native noise calibration has no working public call path here; do not label a no-op as calibration. Donor standby/touch services are not port APIs. Use the real Titan emission control for deliberate output-off, with truthful naming.

For every capability provide: stable ID, label, type, native range/default, UI conversion and inverse, applicability, writable/supported status, state ownership and persistence policy. Do not populate the SMC catalogue with legacy motion/tonal/focus/attack controls that have no active donor consumer.

### Source-grounded quirks that must survive integration

- **Auto-color:** `emotiscope_donor/leds.h::update_auto_color()` increments `configuration.color` without wrapping; rendering wraps hue. The existing full-config setter rejects colour above 1. Therefore a naïve GET-modify-SET eventually rejects unrelated changes, and a stale full-config write can repeatedly rewind the hue. Implement masked field patches in the RT owner. Validate only supplied values and preserve engine-owned evolving fields. Define a normalized effective-hue readback without overwriting the internal phase on unrelated writes. If a boundary normalization is needed, prove modulo-equivalent rendering. Do not advance the config revision on every auto-color frame.
- **Saturation:** native donor commands apply `sqrt(UI_value)`; the port setter takes raw saturation. Retain this physical feel: normalized fader position u maps to raw sqrt(u); pickup/readback uses raw². Default raw .75 corresponds to u .5625. Apply the transform once. Presets store native raw values with explicit schema.
- **Bloom speed:** native `light_modes/active/bloom.h` computes a speed-based draw then overwrites all those pixels before retaining the frame. Expose it as “no effect in this donor Bloom variant”; do not rewrite Bloom to make a pixel-difference test pass. Speed is effective in Analog and is not a tempo/BPM knob.
- **Screensaver:** silence below its native threshold waits 5 seconds, then blends gradually. Disabling it does not currently clear all retained mix/LPF history. Preserve and document this, with discriminating tests. Do not secretly zero histories.
- **Mode changes:** the native setter sets `lpf_drag=1` on a real mode change. Do that once, preserve other histories, and synchronize all mode readbacks/catalogues.
- **Dithering:** a single-byte or mirrored-pixel difference is not automatically a geometry fault; use prequantization geometry or a justified temporal comparison.

## 5. Physical SMC mapping and identity repair

Reuse the existing controller configuration. Do not reprogramme MIDI CC assignments just to simplify host code.

The source inventory is 8 faders, 8 absolute knobs and 45 buttons: 61 physical controls. Fifty-nine have configured identities; BT and SHIFT remain unknown. Do not bind BT/SHIFT or depend on them for any required function.

All MIDI channels below are **zero-based**. Raw absolute controls use 0–127; buttons are press 127 / release 0.

| Physical IDs | MIDI identity |
| --- | --- |
| Fader1–8 | ch0 CC40–47 |
| Knob1–8 | ch0 CC30–37 |
| M1–8 | ch0 CC20–27 |
| S1, S2 | ch0 CC28, CC29 |
| S3–8 | ch1 CC30–35 |
| R1, R2 | ch1 CC36, CC37 |
| R3, R4 | ch0 CC38, CC39 |
| R5–8 | ch1 CC40–43 |
| SQ1–4 | ch1 CC44–47 |
| SQ5, SQ6, SQ7, SQ8 | ch0 CC48, ch0 CC49, ch0 CC51, ch1 CC51 |
| Play, Pause, Record | ch0 CC52, CC53, CC54 |
| Rewind, FastForward, Previous, Next | ch0 CC55, CC56, CC57, CC58 |
| Up, Down, Left, Right | ch0 CC59, CC60, CC61, CC62 |

Current evidence distinction: the USB profile labels 55 identities as captured and four as configured. Those four are Fader1, Fader4, R5 and R8. The capture-hash discrepancy below qualifies the provenance of a subset of those 55. Bluetooth has four independently evidenced identities—Fader7, Knob1, Play and Next—plus 55 configured identities copied from the common mixer configuration. The later source admits configured inputs; the older DASHBOARD prose saying only four BLE inputs are admitted is stale. Do not describe configured identity as independently captured evidence.

A September 28 collision capture also has a provenance discrepancy: current raw SHA-256 is `af294178e01d4121af20b7c4e4573204979e0ce0a7597c6e264b77903c63db9b`, while the profile cites `b83f6ae0733ca2bd43876c05fa1dd5c664eca8f37eac0f22b0fc62dc0e5dd0f1`. Locate these references in the host profiles/reports, preserve both records, explain any append/change and take fresh labelled captures where necessary. Do not “fix” this by silently replacing the expected hash and claiming verification.

Exact capture locations, rooted in `/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/` rather than its worktree:

- `reports/20260927-performance-implementation/midi-capture/sealed/raw.jsonl`: current hash `887489d0a99919621312779673847ba182b3a6496dac90524be7173172c989a3`.
- `reports/20260928-performance-resume/midi-collision-capture/raw.jsonl`: discrepant capture above. It supports R1/R2/R6/R7, S3–S8 and SQ1–SQ4/SQ8; several are required by the proposed layout. Resolve provenance or recapture affected required inputs before calling their path qualified.
- `reports/20260928-dashboard-implementation/bluetooth-four-controls.jsonl`: current hash `306f976712d0f5bac40c54c148f5f379f2c4a8171fe33aa56cec273cb1b734fd`.

Profile sources are `S/host/titan_control/profiles/{performance-v1,surface-observer-v1,configured-inputs-v1,bluetooth-verified-v1}.json`. Actual admission logic is `S/host/titan_control/smc_titan_control/midi_transport.py`; preserve the distinction between configured and captured inputs there.

Before enabling required ambiguous controls, take labelled, isolated post-save gestures with Titan writes suspended but its autonomous show continuing. Prove Fader1 and Fader4 do not alias R5/R8. Include the two R buttons in the same bounded capture when available. Complete raw and decoded event evidence on the actual selected transport. Unused unverified buttons do not block implementation of unrelated controls; Fader1/Fader4 must be resolved for the complete default layout.

Use this **new Emotiscope profile** as the initial layout. Existing user assignments remain preserved separately; do not silently reinterpret their legacy field names.

| Physical control | New assignment | Conversion / behavior |
| --- | --- | --- |
| Fader1 | Donor brightness | u = CC/127, native value u |
| Fader2 | Softness | u |
| Fader3 | Speed | u; show current mode applicability |
| Fader4 | Background | u |
| Fader5 | Color | u; explicit gesture sets hue; auto-cycle continues by native semantics |
| Fader6 | Color range | u |
| Fader7 | Saturation | sqrt(u) native; inverse u = raw² |
| Fader8 | Warmth | u |
| Knob1 | Output brightness | rounded 255 × u; integer readback; pickup against current output value |
| M1–M7 | Modes 0–6 respectively | One selection per fresh press |
| S1 | Mirror | Toggle from confirmed device state, send explicit boolean |
| S2 | Reverse color range | Same; show dependency on nonzero range |
| S3 | Auto color cycle | Same |
| S4 | Screensaver | Same |
| S5 | Temporal dithering | Same |
| Previous / Next | Previous / next musical mode | Wrap only through IDs 0–6 |
| Play | Emission ON | Explicit operator command; preserve current config and output brightness |
| Pause | Emission OFF | Explicit operator command only; never host lifecycle cleanup |
| SQ1–SQ8 | Host scene slots | Fresh press recalls an existing compatible slot; empty slot sends nothing |

Leave other controls unassigned initially; the live assignment catalogue may bind them to supported functions. No required function may depend on an unverified chord, unknown SHIFT/BT identity or a private MIDI endpoint.

Scene saving, naming, defaults restoration and assignment editing may use the existing **functional live controller dashboard**, plus documented CLI operations. This is a control interface, not a substitute visual presentation. Do not create a new preview site. Saving a scene or remapping sends no parameter write. Restoring defaults is explicit and atomic; defaults are the native values plus mode 1, output 100, emission ON. A scene should store the donor config and declared output-brightness setting; emission is excluded from ordinary scene recall. Never recall a scene automatically on connection.

For scene colour, store the normalized effective hue sampled at save time, alongside the auto-color flag. Recall sets that hue once and lets native cycling continue if enabled. Do not serialize an unbounded internal hue into a 0–1 input field. Scenes restore controls, not a frozen AP/VP-history snapshot; retained trails, audio/tempo history and native colour momentum continue.

## 6. Work package A — checkpoint and typed firmware interface

1. Record clean/dirty state and hashes in the existing lane evidence directory. Preserve the current installed HEX/ELF/receipt as recovery input. Do not overwrite `arm-port-004` or its target observations.
2. Read `F/platform/ra8p1/fixture_app.cpp`, `palette_runtime.cpp/.h`, `emotiscope_port.cpp/.h`, `k1_live_protocol.cpp/.h`, `F/docs/contracts/titan-emotiscope-v1.json` and `titan-emotiscope-spectrum-defaults-v1.json`.
3. Add a donor-specific, versioned GET-state / masked-PATCH contract using the existing K1S1 transport and opcode25 namespace. Choose and document an unambiguous donor discriminator/subcommand allocation after checking the existing enum. **Do not simply remove the current opcode25 rejection and feed old K1 requests into donor fields.** Legacy mutations must still reject under the donor profile.
4. Bind engine ID, schema/version, UID/build, boot/session identity, request ID and expected config revision. Define exact sizes, endian encoding, field mask/IDs, native value representation, bounded payload limits and error codes. Reuse existing boot/revision/receipt structures where sound; do not invent a second command framework.
5. GET returns a coherent RT-owned state snapshot: native configured values, effective hue/auto ownership, mode, output brightness/emission, config revision and receipt state. An unrelated patch must not copy stale auto-color or other runtime state back into the engine.
6. Reject unknown fields, nonfinite/out-of-range floats, invalid booleans/enum IDs, wrong lengths/versions, duplicate conflicting entries, stale revision and old boot requests atomically. A malformed patch changes nothing. Retrying the same request must not toggle twice or re-trigger a mode transition. Define bounded deduplication keyed by boot/session/request ID and payload identity: same ID/different payload rejects, retained duplicates return the original result, and expired or unknown outcomes require state reconciliation rather than being blindly re-applied. Document the retention bound and test eviction.
7. Extend the existing fixed `PendingControl`/single RT-owner mechanism. USB parses and queues immutable intent; only the owner applies config while the paired transfer is idle. Do not call donor setters from MIDI, USB callbacks or a second thread. Never mutate a DMA-owned frame or perform file I/O/allocation in the RT path.
8. Keep desired config revision and donor runtime evolution distinct. Patch only touched fields. Apply combined scene changes atomically. Give pending work bounded backpressure/busy responses; host coalescing handles rapid motion.
9. Maintain the distinction between **accepted/applied**, **render consumed** and **DMA completed**. Bind request/revision to the actual rendered frame and completed pair. A no-op or superseded request must not be falsely credited with a new optical result. Expose unsupported/no-effect applicability independently.
10. Integrate output brightness/emission through their existing consumer, preserving single gain and autonomous defaults. In the new controls-enabled donor profile, retire the old opcode16 mutation path and advertise the new typed mode/output controls instead. Explicitly reject the old 32-byte configuration write; preserve read-only observations and non-donor profiles. This intentional schema-bound change prevents unversioned mode/output writes from bypassing the new boot/revision guards or creating a second unsynchronized state. Update clients and regression expectations together.
11. Update the profile schema, capabilities, catalogue, build hash inputs, codecs and host decoding together. The build script generates this profile’s schema include in the stage directory. Do not modify the legacy include to impersonate the new schema. Advertise only implemented features.
12. Preserve native scalar response first. Do not reuse legacy 80/100/120 ms ramps or 750 ms audition leases automatically. If measured MIDI stepping requires smoothing, implement it only for explicitly named continuous donor fields, document its target/current values and settling bound, and show the unsmoothed baseline remains selectable/testable. Native smoothing and mode transition already exist.

Output of A: executable typed controls with meaningful host tests, versioned schema, coherent source-bound state and receipts. This is the prerequisite for enabling the live host writer.

## 7. Work package B — broker, SMC owner and usable state

Reuse the existing ownership path:

- `F/tools/serial-studio/titan_broker.py`: persistent CDC owner and lease.
- `F/tools/serial-studio/titan_control_server.py` and `live_protocol.py`: local IPC translation and codecs.
- `S/host/titan_control/`: existing `smc_titan_control` entry point, `performance_live.py`, `performance_input.py`, `mapped_performance.py`, `performance.py`, `midi_transport.py`, assignment/profile and dashboard modules.

Do not route live fader events through `F/tools/control-surface/server.py`’s per-request CDC/full-config path. Do not start a second serial broker or parallel MIDI owner.

Required implementation:

1. Introduce an explicit Emotiscope strategy/profile keyed by the returned engine/schema. Keep the parked-engine strategy distinct. An old config, old scene or unknown schema cannot become a valid donor patch by matching a label or ordinal.
2. On attach, reconnect, target reboot or transport change: verify identities, fetch actual target state, clear stale in-flight intent, refresh config revision, reset pickup and require held buttons to release. Do not send defaults, scene recall or the physical faders’ initial positions.
3. Preserve the current maximum 25 Hz writer, one IPC transaction per tick and at most eight fields per ordinary batch unless tests justify a bounded change. Firmware/IPC atomic scene recall must fit the complete config, without pretending several independently applied batches were atomic. Make selection fair across active fields, discrete actions and state reconciliation: dictionary order is not a fairness guarantee. Test maximum pending age and eventual delivery of each final value during simultaneous motion; a continuously changing early field must not starve later fields or buttons.
4. Preserve the bounded 512 gesture-group inbox, 32 raw-log entries and mapping-generation fences. Coalesce continuous inputs by field with latest-value semantics; preserve discrete press/release order. Quantify overflow/supersession; never hide lost button releases or replay an old generation.
5. Absolute faders/knobs use soft pickup with the existing ±1/127 or crossing rule in **physical UI space**. Use the saturation inverse above and proper integer brightness conversion. Remapping/reconnect resets pickup. Auto-moving hue needs an explicit rule: pick up against a displayed sampled target, capture ownership on crossing, send hue only on actual movement, and do not chase evolving readback with repeated writes.
6. Buttons trigger once per fresh press, are idempotent at the request layer and derive toggles from confirmed state. Preserve discrete intents in order and resolve each toggle/Next/Previous against the confirmed result of the preceding intent; two presses before an acknowledgement must not both compute the same stale target. If state is UNKNOWN after a timeout, reconcile before the next toggle; do not guess or issue blind retries. Held buttons cannot act on attach/reassignment. Test rapid double presses, mixed scene/mode/toggle actions and timeout reconciliation.
7. Preserve endpoint identity checks and exactly one selected input. USB endpoint is `SINCO SMC-Mixer-Master`, driver `com.apple.AppleMIDIUSBDriver`, unique ID `1873467316`. Supported Bluetooth endpoint is `SMC-Mixer Bluetooth`, driver `com.apple.AppleMIDIBluetoothDriver`, unique ID `-2057804531`, peripheral ID `9B4CCE67-9BA5-456D-EF69-451FC96D45F1`. These are recorded identities to recheck, not permission to bypass a mismatch. The Private endpoint is not a fallback. Bluetooth also binds the admitted connection epoch; use the existing monitor. A BLE event stream still requires Titan’s USB CDC broker.
8. Maintain existing loopback dashboard access controls and bounded request/disk workers. Show physical raw input, assignment, pickup/release-needed state, engine/mode, applicability, confirmed target/native value, pending value and uncertainty. Show render/transfer proof where available without calling it optical proof. Hide legacy unsupported AP metrics rather than showing fabricated zeros as music intelligence.

The 32-entry live log is a display bound, not the evidence recorder. Retain complete raw/decoded records for each bounded qualification session through a bounded host logging path; report any dropped records and invalidate affected claims. Do not perform synchronous disk writes in MIDI callbacks or the RT owner.
9. Version and atomically persist Emotiscope assignments and host scenes. Preserve current legacy files and last-good backups. Reject incompatible/corrupt schema; do not auto-migrate legacy “motion” into donor speed. Store native saturation, not its UI-domain value. Never claim MCU NVS persistence: port save functions are stubs.
10. Remapping, save, host exit and transport loss must not change emission or reset donor state. Update old host health assumptions so an acknowledged deliberate operator emission-off or dimming command is displayed accurately, not treated as a reason to auto-rearm or reject all subsequent controls. A short bounded activity/wake hook is optional only if a concrete usability defect requires it; it is a separately declared adaptation, with no fabricated physical touch or permanent touch latch.
11. Supply one exact launch command or small launcher that resolves the correct repo, schema, profile, lease and port. The user should not assemble several undocumented processes.

Existing host command shape, to adapt after implementing the strategy:

```sh
cd /Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001
PYTHONPATH=host/titan_control host/titan_control/.venv/bin/python \
  -m smc_titan_control live --strategy performance \
  --socket /tmp/titan-perfclass28/titan-control.sock \
  --port 'SINCO SMC-Mixer-Master'
```

This is the retained old-strategy command, **not a claim that Emotiscope currently works with it**. Publish the exact new invocation after implementation. The existing dashboard is on loopback port 8776 and status on 8766; inspect ownership before reuse. Use the repository virtual environment, not an unrelated global Python. Keep Unix socket paths short under `/tmp/`; a deep worktree path previously exceeded macOS AF_UNIX limits.

The existing broker command shape is `S/host/titan_control/.venv/bin/python F/tools/serial-studio/titan_broker.py run --evidence /tmp/titan-emotiscope-control --live-control --continuous-output`. Resolve the F/S prefixes to absolute paths. Broker and controller are separate long-running processes, or children of one explicit launcher—not sequential commands in one blocking shell. Read and satisfy the existing candidate/admission record and `TITAN_LIVE_CONTROL_ADMITTED` mechanism using the actual new image/schema and standing authority. Do not manufacture a PASS admission, disable the guard, or point it at the old image. The writer socket for that example is `/tmp/titan-emotiscope-control/titan-control.sock`.

If using Bluetooth, the retained helper build is:

```sh
/usr/bin/swiftc -O host/titan_control/tools/ble_monitor.swift \
  -o host/titan_control/tools/smc-ble-monitor
```

Do not scan, pair or reconfigure the mixer to conceal a transport identity problem. Continue with an already available admitted transport and report the other as not physically tested.

## 8. Work package C — focused verification before deployment

Use existing tests and add only meaningful cases for the new control boundary. Preserve prior failures and use fresh evidence directories.

Existing port regression entry points:

- `F/scripts/test_emotiscope_port.py`
- `F/tests/host/test_emotiscope_visual_port.py`
- `F/scripts/test_emotiscope_output.py`
- `F/scripts/test_emotiscope_live_bridge.py`
- `F/tests/host/test_emotiscope_capture_adapter.py`

Use their actual CLI/help and repository test runner; do not invent arguments. Run the host controller suite around the changed strategy, mapping, persistence, broker and reconnect code. Preserve the existing legacy-profile regression coverage for shared changes. Re-run AP/capture arithmetic parity if those consumers change; a host-only mapping edit does not require gratuitously re-running every historical long fixture.

Verified existing host test entrypoints (not executed during this brief’s preparation) are `S/host/titan_control/run_tests.py`, unittest discovery under `S/host/titan_control/tests`, `F/scripts/test_live_protocol.py` and `F/tools/serial-studio/tests/test_titan_control_server.py`. Use `S/host/titan_control/.venv/bin/python` and `PYTHONPATH=S/host/titan_control` where required. Relevant suites include bounded MIDI, performance, assignments, dashboard, Unix-broker lease, negatives, review regressions and Titan live-control-path tests. Do not cite their historical passing counts as this candidate’s results.

Required checks:

| Area | Required proof |
| --- | --- |
| Contract | All fields’ endpoints/midpoints and boolean states round-trip; malformed/stale/wrong-engine requests reject atomically |
| Ownership | Patches apply only in the owner at safe boundaries; no engine reinit, changed sample accounting, DMA overlap or unbounded allocation |
| Auto-color | Run across hue wrap; unrelated patches continue succeeding without freezing/rewinding the phase; explicit color control works |
| Saturation | UI→native→UI round-trip; presets/readback do not apply sqrt twice; default remains raw .75 |
| Donor fidelity | Same PCM/seed/timestamps/default config produces unchanged output; representative changed controls match native behavior |
| Applicability | Analog speed changes transient response; Bloom speed is faithfully ineffective; other inapplicable cases are correctly identified |
| Colour/geometry | Nonzero range/saturation discriminates mirror/reverse/hue; continuous160 and 80+80 split stay correct; gain is applied once |
| Screensaver/dither | Silence beyond native delay and gradual recovery; fractional multi-frame dither fixture; no convenient history reset |
| Mode/scene | All seven musical modes; single native transition; atomic recall; mode7/8 unavailable in musical selector |
| Host safety | Pickup, release-required, button edges, generation fences, UNKNOWN reconciliation, boot/schema/UID mismatch, bounded flood and remap |
| Persistence | Explicit saves atomic; compatible reload exact; corrupt/legacy scenes reject; attach itself sends no patch |
| Provenance | Required physical identities labelled and raw capture hashes match; configured and observed status remain distinct |

Include targeted negative proofs: old legacy packet cannot mutate donor state; wrong boot/schema cannot write; stale full-config copying would fail the auto-color test; doubled saturation transform fails its round-trip; a forced engine reset fails history preservation. Reuse existing gain/split mutations rather than duplicating them without a changed risk.

Do not require pixel differences where native behavior predicts none. Conversely, an acknowledged field or green test that merely echoes the submitted value is insufficient consumer evidence.

## 9. Work package D — exact build, programme and target controls test

Before target access, inspect actual processes and gracefully hand off the current CDC owner using the existing lease workflow. No unrelated process termination. Release both the serial handle and its flock when relinquishing ownership; closing serial alone is insufficient.

Keep the working image running during host development. Do not stop the show to wait for code or a mixer capture.

The verified port build shape is below. This is a command template: choose a fresh output directory and include any deliberately updated schema/default inputs in its receipt.

```sh
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001
python3 scripts/build_scalar.py \
  --output docs/emotiscope-port-001/evidence/arm-smc-controls-001 \
  --pdm-target --palette-runtime --palette-autostart --palette-morph \
  --palette-ws2816-gpt-pair \
  --dmac-lane-map led-first --dmac-priority fixed \
  --live-runtime --live-emit --tempo-field --emotiscope-port \
  --live-defaults docs/contracts/titan-emotiscope-spectrum-defaults-v1.json
```

The retained `--tempo-field` flag satisfies existing compatibility staging; it is not authority to restore the legacy AP. Retain compiled active-call checks for donor AP/VP, scalar build settings, no RT allocator, stack/link-map checks and exact source/schema hashing.

After required host checks and ARM build pass:

1. Read current UID/build/configuration under the existing lease. A different build may reflect legitimate newer work; resolve its provenance before overwriting it. Unknown UID or unexplained state stops programming.
2. Verify candidate HEX/ELF hashes against its fresh receipt. Use only `scripts/programme_rfp_swd.py` through the established router. Supply that receipt’s exact source/build/HEX/ELF identities.
3. Preserve E2 Lite serial `5AS079228B`, SWD1MHz, probe target power OFF, `-noquery -run` and permitted range `02000000,02C9F01F`. No GDB, serial-ROM programming, chip erase, configuration-area writes or vendor BSP edits.
4. Read full UID/build/schema/boot/config after application return. Verify Spectrum/default100/emissionON and advancing valid audio/render/output before opening the new writer.
5. Run the new host with the exact admitted engine/schema/profile. Take real mixer gestures, record raw MIDI through acknowledged patch, owner application, rendered frame and completed pair. Then observe the LEDs with the operator.

The canonical writer accepts this argument shape; substitute only fresh receipt values:

```text
K1_RFP_SWD_EXECUTE_OK=1 python3 scripts/programme_rfp_swd.py
  --build <fresh-build-directory>
  --output <fresh-programme-directory>
  --expected-uid 545433931bd25436593630352d068363
  --expected-source <receipt-source-pin>
  --expected-build-id <receipt-build-id>
  --expected-hex-sha256 <receipt-hex-sha256>
  --expected-elf-sha256 <receipt-elf-sha256>
  --execute
```

Use the exact script help/router invocation rather than treating this wrapped text as a shell command. Deployment authority comes from the explicit 27 September SMC–Titan standing instruction retained in `F/AGENTS.md`, as updated by the 29 September Emotiscope/brightness/geometry directives—not from this brief inventing permission. It covers the existing SMC host and Titan worktrees, repair firmware, programming and bounded physical verification. No further per-image approval is needed within that scope; any later user HOLD still overrides it.

### Physical test sequence

Use bounded sessions of at most 120 seconds under the standing bench rules. Ending a session ends collection only; it does not disarm. Keep output brightness100 for automated stress/comparison; full numeric endpoint validation can remain host-side, and deliberate operator changes retain their explicit values.

1. **Baseline:** use the retained Sonic Samba and Carte Blanche windows under `F/docs/emotiscope-port-001/evidence/audio-windows-001/`. Preserve their manifest identities. Observe valid advancing audio and changing donor output before fader input.
2. **Controls:** after required MIDI identities are qualified, cue a short numbered sweep through eight faders, five toggles, all seven modes and Knob1. Use discriminating modes/settings—for example Analog for speed and nonzero color range for reverse. Record applicability and exact state before/after each. Test explicit Play/Pause as deliberate controls only; restore emissionON. Do not treat accidental darkness as a successful test.
3. **Scenes/assignment:** save a named host scene, change controls, recall it, verify atomic state and preserved audio history. Remap a spare admitted input, prove pickup/release gating and no write merely from remapping, then restore the agreed mapping.
4. **Load:** replay a recorded realistic multi-control burst through the production input/owner path, alongside live music, with source-labelled synthetic input for automation. Measure its input rate, coalescing and bounded backlog. Cover sustained 25 Hz command batches and simultaneous telemetry; do not claim this is a human gesture capture.
5. **Reconnect:** selected MIDI disconnect/reconnect, owner restart and lease release/reacquire must clear pickup/held state, reconcile the board and send no unsolicited reset/default/off command. Where the real transport interruption cannot be executed, separate host simulation from NOT TESTED physical coverage. No cold power-cycle experiment is added by this brief.
6. **Leave-state:** check the exact image, mode/config, brightness and emission after the bridge relinquishes its lease or reconnects normally. Verify analysis/render/output continue independently. Leave the usable controller attached if intended, music visualization active, no test fixture armed.

Retain the existing read-only observer `F/scripts/observe_emotiscope_target.py` and adapt only the necessary schema/controls awareness. Its normal invocation includes `--output`, `--expected-build`, `--seconds` and optional `--audio`. It verifies wire mapping and releases serial plus flock. Do not run it as a competing CDC owner while the broker is active; expose equivalent observation through the owner or perform a clean handoff.

Measure control latency from host event through acknowledgement and completed transfer using the appropriate monotonic clocks. Report p50/p95/max and uncertainty; never subtract unrelated host/target clocks. Derive the expected bound from the 25 Hz owner, measured transaction time and 10 ms render schedule before evaluating it. Do not promise optical/audio synchronization from USB timestamps.

## 10. Acceptance, failure handling and necessary follow-through

| Criterion | Pass condition | Evidence |
| --- | --- | --- |
| Complete supported surface | All14 native fields, seven musical modes, separate output brightness and explicit emission reachable; unsupported facilities honestly labelled | Capability manifest, default mapping, state/consumer tests, live demonstration |
| Correct physical identity | Every control used in the default layout resolves to its labelled source on the selected transport; no unresolved collision can write | Fresh raw/decoded captures, source identities and matching hashes |
| Faithful behavior | Default rendering unchanged; controls preserve native math/history and documented no-op cases | Deterministic comparisons, auto-color/saturation regressions |
| Safe ownership | One CDC/MIDI owner; atomic validated patches; no stale-boot or wrong-schema write; no unexpected reconnect write | Broker/controller tests and actual connection logs |
| Audio/render continuity under controls | Advancing audio/render/pair counters; no new capture loss, queue rejection, stale transition, LED fault or emission error attributable to control tests | Before/after counters plus complete relevant timing/log windows |
| Timing | No observed AP execution over 5000 us in scored runs; VP cost below 10000 us; report actual schedule gaps and freshness separately | Timing history with coverage/gap accounting, measured maxima and counts |
| Buffer correctness | Current160/80+80 GRB48 mapping and gain-once invariant preserved for changed configs | Actual buffer/wire readback plus independent calculation |
| Persistence and usability | Compatible scene save/recall and assignment restore work; startup/reconnect reads state without overwriting it | Files/schema plus live and host results |
| Deployment | Exact candidate is programmed and reports matching UID/build/schema; normal boot/application return emits autonomously | Build/programme/readback receipts |
| Operator result | User can manipulate controls and inspect the actual LEDs; any aesthetic complaint recorded separately | Short physical-use verdict, never inferred from counters |

The last port’s two short trance runs reported zero new capture/LED faults, AP maxima around 1.46 ms and VP below 0.75 ms, followed by a short autonomous-disconnect observation. They are baseline evidence, not a substitute for control-load tests, long-duration reliability or absolute optical timing. User-observed stability is encouraging but does not close those untested claims.

**Gates and resumption:**

- Identity/capture uncertainty: disable only the affected input writes, preserve the show and continue other implementation. Resume that control after labelled evidence resolves it.
- Contract/host failure: keep new writer disabled; repair and rerun the failed relevant tests. Do not flash a known failing boundary.
- Unknown target/hash, failed ARM check or unavailable required programming tool: no programme. Preserve current show and candidate; state exact missing prerequisite.
- New audio loss, deadline failure or stalled rendering during load: preserve first-fault evidence, stop the offending control/diagnostic load and repair its cause. Do not routinely black out healthy autonomous rendering or hide discontinuities. A genuine hardware hazard follows the existing fault-safe procedure.
- A failed candidate may be recovered through the established identity-gated path using the preserved **current Emotiscope baseline**, after resolving any fault latch/recovery requirements. Do not automatically restore the rejected Aurora archive or re-arm an unsafe image.
- Actual user optical judgement belongs to the user. Complete all technical work and cue a short concrete mixer exercise when ready; do not make the user diagnose protocol or source defects.

Necessary remaining work belongs in this execution: auto-color-safe patching, honest saturation conversion, native mode/state synchronization, new schema and profile, provenance/collision repair for required mappings, reconnect/pickup behavior, host persistence, live control-load verification and a usable launcher. These are not optional future polish.

Long-duration unattended reliability, absolute acoustic-to-light latency calibration, other physical geometries, donor utility services and legacy contract closure remain explicitly outside this bounded controls delivery unless a concrete failure makes one a prerequisite. Do not reopen resolved LED/DMA architecture on the basis of old reports.

## 11. Deliverables and Grok closeout

Write implementation and evidence into the existing two worktrees. Suggested new lane folder: `F/docs/emotiscope-port-001/smc-controls/`; keep candidate evidence under fresh numbered directories. This is a proposed destination, not an existing completed result.

Deliver:

1. Narrow source commits or exact owned diffs in both repos, with actual dirty-source identities and donor deltas if any.
2. Versioned control/schema and applicability inventory, physical assignment table, persistence format and exact launch/stop/reconnect instructions.
3. Relevant test results including negative cases, raw MIDI qualification, build/programme receipts and image-bound physical control/timing results.
4. Updated `F/docs/emotiscope-port-001/CURRENT-STATE.md` and SMC live-controller documentation. Remove contradictory active instructions by explicit supersession, preserving historical receipts.
5. A one-page operator control card: which fader/button does what, pickup behavior, which controls are conditional, and how to save/recall a scene. It must describe the installed mapping.
6. Final factual report: current resident build/HEX/schema, active mode/brightness/emission, running broker/mixer endpoint, PASS/FAIL/BLOCKED/NOT TESTED for each criterion, unresolved faults and exact next action/owner.

Do not claim “all controls work” from a mock, a command echo or a GUI display. Do not claim aesthetic improvement when the task was faithful controllability. The successful handback is a Titan the user can immediately play through the SMC-Mixer, with the original Emotiscope musical behavior still running.
