# Captain host repair — 29 September 2026

## Result and current operation

The running controller is attached to the existing SMC-Mixer Bluetooth endpoint and the sole existing Titan broker. The live dashboard is http://127.0.0.1:8776; status is on 8767. Port 8766 belongs to an existing static server and was left alone. No firmware was rebuilt, flashed or reset during this repair. No audio playback was started or replaced, no Bluetooth pairing was performed, and no automatic emission-off command was sent.

Verified resident build: `393527e4ff81eb5c4fe54e8be3bf874bc37dce9fd89a2d9fec3d4b3e7ac8c4cb`.
Schema: `5f3c9fa627067bf3357bdb142b4d4efcb8d16211164431864826af9102286f60`.
UID: `545433931bd25436593630352d068363`.
Current live mode: Spectrum; output brightness 253/255; emission True; config revision 372.

The parser and tempo-link changes were already on disk when independently inspected; the other session had progressed beyond its pasted handback. They are not claimed as Captain-authored repairs. The broker readback confirmed the exact image and control admission. A historical checkpoint field remains unaccepted; it was not relabelled as a hardware PASS.

## Owned repairs

1. Fresh button edges: repeated down packets cannot produce multiple toggles without release.
2. Ordered Previous/Next: mode stepping resolves after preceding scene/default/mode acknowledgement, not against stale arrival-time state.
3. Fader1/Fader4: unlabelled channel-0 traffic cannot qualify or arm them. Isolated labelled Fader1/Fader4/R5/R8 evidence is required and scoped to the current transport epoch.
4. Persistent wire IDs reserve disjoint blocks across host restarts; the production controller does not reset its request counter to 1 and collide with firmware deduplication.
5. Bluetooth connection freshness and epoch fence: discard queued intents, reset pickup/held buttons, and read the actual target before resuming after loss or epoch change. No output-off action is attached to host lifecycle.
6. One Emotiscope MIDI owner lock, explicit writer-busy rejection, bounded recent sent-batch history, visible connection/loop errors and resource cleanup.
7. A labelled capture UI records raw CC messages before coalescing. Wrong/mixed identities, incomplete sweeps, timeout and recorded overflow cannot qualify the affected input.

## Executed verification

- 39 targeted strategy, new regression and no-hardware-import checks passed in the production Python 3.13 virtualenv.
- The full pytest collection passed: **155 tests**, Python 3.14 / pytest 8.4.2, with the existing pure-Python mido package explicitly loaded from the controller environment. No packages were installed, and no compiled MIDI backend was imported into that test interpreter. `test-environment.json` records this distinction.
- The unmodified virtualenv-wide unittest invocation still reports two pytest import errors and one separate browser skip; its log remains retained. Those are not erased by the successful complete collection in the documented test environment.
- The independently executed live-protocol host script exited 0. Its schema belongs to that test, not to the resident image.
- Identity-checkpoint and control-server scripts passed. The platform-memory structural check passed; that check is not new silicon verification.
- A source-labelled synthetic Fader2 gesture passed through the actual production engine, broker and target. Softness changed from 0.25 to 33/127 and was restored to exactly 0.25. The change and restoration have matching accepted, rendered and emitted request IDs in `target-synthetic-softness-001.json`.
- One observed change acknowledged after 7.444 ms; matching render/transfer confirmation was observed by 58.237 ms, including polling delay. These are one sample, not p95 or optical latency.
- A subsequent 20-second polled observation showed continuing audio and LED progress, brightness100, emissionON and no new reported LED pair/emission faults. It is not a complete raw MIDI recording or optical observation.

## Explicit remaining boundaries

No human mixer gesture was received in the recorded window. Full physical qualification of all eight faders, five toggles, seven modes, output brightness and scene actions is **not complete**. Fader1/Fader4 remain gated pending the labelled collision captures; other configured mappings are not promoted to captured evidence merely because they are usable. USB was absent; only the already-connected, exact-identity Bluetooth transport was used.

The last attempted refinement to capture-finish persistence ordering and the launch string was blocked by a tool safety check and was not applied or retried through another route. The qualification manifest still needs that hardening review before final release qualification. Do not call this whole controls assignment accepted. The dashboard status JSON's legacy `launch` string still describes USB/8766; use the actual invocation below instead. The first launch briefly failed endpoint admission, while subsequent independent reads matched the exact online endpoint and the next launch succeeded; startup-discovery reliability is not proven by this run.

Optical quality, human control usability, long-duration reliability, full control-load timing and physical reconnect testing remain unproven. Preserve the user's actual judgement of light. Do not claim all controls work from the synthetic transaction.

## Actual launch / stop

The existing identity-admitted broker at `/tmp/titan-emotiscope-control/titan-control.sock` must already be running. Do not start a second CDC owner. The controller currently remains running; a second launch is refused by its MIDI owner lock.

```sh
cd /Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001
PYTHONPATH=host/titan_control host/titan_control/.venv/bin/python -m smc_titan_control live --strategy emotiscope --socket /tmp/titan-emotiscope-control/titan-control.sock --port 'SMC-Mixer Bluetooth' --dashboard-port 8776 --status-port 8767
```

To stop this controller from its owning terminal, use Ctrl-C. It closes the MIDI/IPC owner without stopping autonomous Titan output. Do not stop the unrelated static server or the existing broker as cleanup. This repair did not push commits or alter other board repositories.

## Evidence

This directory retains the preexisting dirty patches, owned-file before snapshots, reproduced failures, independent protocol log, target transaction and restore, observed controller window, final live state, full test logs, test interpreter provenance, source hashes and owned diffs. Earlier failing logs remain historical evidence.


## Subsequent live mixer observation — supersedes the earlier empty input window

After the original 20-second empty-input window, the production MIDI input received actual CC events from SMC-Mixer Bluetooth. `final-live-state.json` retained Fader8-mapped CC47, Knob1 CC30 and S2 CC29 traffic, while configuration revision advanced from2 to372. Later recorded revision is 502; accepted request 1073808108, rendered request 1073808108, completed request 1073808108. These observations are separate from the source-labelled synthetic Fader2 test.

Latest mode Spectrum, output brightness 255/255, emission True; controller ready True, loop healthy True. The output level changed through live Knob1 input and was not automatically reset to100. Normal default100 is unchanged. Latest reported pair fault 0, emission errors 0.

Real input and actual target control consumption are now observed. This still does not qualify every physical label, establish all controls or prove optical quality. Fader1/Fader4 remain gated. See `real-midi-followthrough-001.json` and `final-live-state-after-midi.json`.
