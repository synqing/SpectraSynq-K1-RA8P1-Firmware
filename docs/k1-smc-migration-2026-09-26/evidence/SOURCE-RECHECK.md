# Source recheck for the SMC migration addendum

**Date:** 26 September 2026, Australia/Perth.  
**Method:** Remote Desktop Commander source/document reads and read-only Git/hash inventory.  
**Inventory timestamp:** 2026-09-26T02:58:27.683188+08:00.  
**Limit:** These were sequential reads, not an atomic filesystem snapshot. No product source was edited, no test or firmware build was run, no MIDI/CDC port was opened, and no target was programmed. Revalidate actual bytes at execution.

## R1 — The recovered lane excludes the retired surface

Read in the existing SMC feature worktree:

- `docs/execution/SMC-TITAN-CONTROL-001.md` (full 246-line brief)
- `docs/execution/SMC-TITAN-CONTROL-001-DISPATCH.md`
- `docs/execution/SMC-TITAN-CONTROL-001-GATES.md`

The mission is the owned commercial SMC-Mixer through macOS CoreMIDI, a host-side sub-lane, with Titan retaining audio/render/output. The brief excludes K1.Sliders, DualMCU, PCB/BSP, ESP32 central and Blender implementation. The new user direction selects USB Master only, so the older brief's optional Bluetooth alternative is not part of this assignment.

The gate file holds CDC/live PATCH pending current admission and candidate identity. It does not authorise programming, reset or emission probes. No new admission was issued by this document-only task.

## R2 — Actual worktrees and content identity

SMC root:
`/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001`

- Branch: `feat/smc-titan-control-001`
- HEAD: `4935fff005768655f5800106616f7a0e86feee6a`
- Git status returned untracked `docs/evidence/`, `docs/execution/`, and `host/`.

Titan root:
`/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-live-control-001`

- Branch: `feat/titan-live-control-001`
- HEAD: `8427fccfb1c3b7fface2c2ba0a56a6017b785acf`
- Modified: contract JSON, fixture application, C++ live codec/header/schema, scalar build wrapper, live protocol tests, import verification, host test and Python protocol.
- Untracked: lane evidence, schema generator, repo-layout helper, apply-patch test helper, control-server tests and `titan_control_server.py`.

Selected hashes, directly calculated on the Mac:

| File (relative to its worktree) | SHA-256 |
|---|---|
| SMC `host/titan_control/profiles/smc-mixer-master-v1.json` | `c849995214c949d1315d3688b844b5b20572ae3fdc89898148d816ac123fc19f` |
| SMC `host/titan_control/profiles/mode-applicability-v1.json` | `947434689e4c7e68659cccbe716ea38ce15880c9f58ccb8992b660c41bac2901` |
| SMC `host/titan_control/smc_titan_control/cli.py` | `516e924925ab95bdd6981f1b726a25520043451da0717d3755e1f4a3fa360654` |
| SMC `host/titan_control/smc_titan_control/adapter.py` | `f30022ce9eda8c95433f0f2916687d9584a9e34dff15421081bdd646876ab7d7` |
| Titan `tools/serial-studio/titan_control_server.py` | `6c771391fd848277ca311bb386cf4e21c3ef31b6af8715176c96b703a6a2e435` |
| Titan `tools/serial-studio/titan_broker.py` | `abcfb0971732dfacd9372ee176021ac27d803490142681998e0593c0c080cce4` |

The broker was hashed, not fully audited here. These six hashes are selected anchors, not a transitive build manifest or running-image identity.

## R3 — Important progress since the retained agent handback

The retained report says a live daemon/timer/UI launcher and Unix socket were missing. Current source differs:

| Source read | Observation | What is not established |
|---|---|---|
| `cli.py` | A `live` subcommand now wires the adapter, `UnixBroker`, explicit MIDI opening, `RateLoop`, and `make_server(adapter.status)` | Successful live session, coherent lifecycle or target control |
| `live_loop.py` | Periodic owner-thread loop exists; CLI default is 25 Hz | Bounded real backpressure, worker-exception health or measured rate |
| `live_midi.py` | Explicit Mido input opening via exact-port selection exists | Physical reconnection handling, source identity and no loss under real load |
| `titan_control_server.py` | `AF_UNIX` listener, socket permission 0600, session and request handling exist | Full persistent broker integration, owner-only directory, robust writer lease or admission correctness |
| `adapter.py` | Initial config checks, in-flight status, acknowledgement checks and mode filtering exist | Exhaustive identity/schema validation, safe uncertainty handling, no stale replay |
| `tests/test_review_regressions.py` | New rejection/readback/applicability tests exist | They were not run for this addendum |

The server's schema comparison is conditional in the source read; nonempty identity fields do not prove a matching admitted target. The client opens a Unix connection per request while server ownership is tied to connection identity. Those are integration review targets, not reproduced failures in this turn.

The real rate loop calls `tick()` without a local exception handler; the UI is served separately. Test that a worker fault cannot leave a healthy-looking but dead control path. Do not call this a observed physical failure.

`Adapter._resync()` resets pickup but the read implementation does not indiscriminately clear its pending coalescer there. Separate legitimate same-session backpressure retention from obsolete-intent disposal after an external edit/session or applicability change. Test the actual behaviour; do not translate the instruction into dropping all final values.

## R4 — New first path is motion, not the old hue-first example

Current `profiles/smc-mixer-master-v1.json` selects:

```json
{
  "source": {"type": "pitchwheel", "channel": 6},
  "target": "visual_a.vp_waveform_shift_rate",
  "range": [0.0, 240.0],
  "role": "motion",
  "required_mode": 8,
  "probe_values": [40.0, 200.0]
}
```

Zero-based channel 6 is physical fader 7 in the captured fader ordering. Mode 8 is a Titan visual-effect mode, not a command to change the mixer's hardware mode.

The referenced raw renderer result was read at:

`/Users/spectrasynq/Library/Logs/SpectraSynq/SMC-Mixer/20260926-023033-independent-review/renderer-results.txt`

Its text:

```text
HUE_AUTO_SHIFT_OFF mode=32 changed_bytes=0 baseline_nonzero_bytes=51118 total_bytes=86400
HUE_AUTO_SHIFT_ON_POSITIVE_CONTROL mode=32 changed_bytes=69604 baseline_nonzero_bytes=51118 total_bytes=86400
FALLBACK_NOT_MASTER_LIGHT mode=32 changed_bytes=0 baseline_nonzero_bytes=51118 total_bytes=86400
MOTION_MODE32 mode=32 changed_bytes=0 baseline_nonzero_bytes=51118 total_bytes=86400
PERSISTENCE_MODE32 mode=32 changed_bytes=0 baseline_nonzero_bytes=51118 total_bytes=86400
SENSITIVITY_MODE32 mode=32 changed_bytes=0 baseline_nonzero_bytes=51118 total_bytes=86400
MOTION_MODE8_POSITIVE_CONTROL mode=8 changed_bytes=11562 baseline_nonzero_bytes=1020 total_bytes=86400
PERSISTENCE_MODE8_POSITIVE_CONTROL mode=8 changed_bytes=0 baseline_nonzero_bytes=11590 total_bytes=86400
```

This is retained software-renderer evidence, not a test rerun, target measurement or optical observation. It does not establish the exact resident mode, an appropriate unattended scene change, or that a no-difference result holds under every stimulus.

The current applicability table permits mode-8 motion and mode-32 hue only when `auto_colour_shift` is already true. Other listed mappings are unproven/disabled for those inspected paths. Fallback brightness is explicitly not a global master light control. Keep this scope: do not reinstate all eight as effective from the old report's codec tests.

## R5 — Old-surface guarantees retained; old-surface work not imported

The earlier profile/stack/no-replay findings remain valid historical evidence for K1.Sliders. They do not establish that the SMC host has the same C++ defect, nor require repairing it before SMC work.

Retain the guarantees—atomic updates, valid revisions, bounded workspace, one writer, no unsolicited writes, coherent requested/acknowledged state, safe recovery—and verify them where they now live. Do not import the old ProfileStore or K1-CONSOLE identity into the commercial mixer merely to preserve an obsolete implementation path.

## R6 — Report and references

`reference/Pasted text(20260925-182715).txt` is an unchanged copy of the saved SMC agent handback. It reports the original receive-only capture, offline implementation and unrun target/physical gates. Use this report for its dated scope and compare with the newer source observations above.

The unchanged complete 56-task technical handoff is retained under `reference/K1-Implementation-Handoff.md`. Prior 2.41-directed briefs are under `reference/previous-direction/` solely to preserve history; they do not override the migration addendum.

The full SMC brief/dispatch/gates remain required at the exact worktree paths in R1. They were read, but their raw copies are not bundled in this package. The raw MIDI capture and target build artifacts are also not bundled. Their location is in the original SMC brief and saved report. This package is a continuation handoff, not a complete build or evidence export.
