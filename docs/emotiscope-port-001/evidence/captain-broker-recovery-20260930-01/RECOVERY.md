# Fader outage recovery — 30 September 2026

## Observed fault

The MIDI/dashboard controller PID79233 was still alive. Its broker socket returned connection refused. The old broker PID36871 was absent; the saved live-stats file was last written on29September22:07:31Australia/Perth and was not current target state. There is no retained shutdown receipt establishing why that process terminated. Do not infer a firmware or physical fader fault from that dead host connection.

## Action and verification

Restarted the exact existing broker with its unchanged image-bound admission, existing socket/evidence path, live-control and continuous-output flags. It is now PID32626, launched with a new session, closed inherited descriptors, null stdin and persistent console logging rather than tied to the prior agent terminal. The parent launcher exited; ps subsequently showed parentPID1 and process-group32626. No automatic restart service or login agent was installed.

The existing controller reattached without being restarted. Fresh readback matches UID545433931bd25436593630352d068363, build393527e4ff81eb5c4fe54e8be3bf874bc37dce9fd89a2d9fec3d4b3e7ac8c4cb and schema5f3c9fa627067bf3357bdb142b4d4efcb8d16211164431864826af9102286f60. No programming, reset, scene/default application or output-off command was sent. Existing Hype mode, output253/255 and emissionON were preserved at reconnection.

A bounded20second observation recorded advancing AP hops and completed LED frames with pair fault0 and emission errors0. Actual Fader2 CC41 messages arrived after reconnection. They stayed in pickup; no new applied control revision was observed in that window. This is verified link recovery and input reception, not a new optical or all-fader acceptance claim.

## Last readback

Time: 2026-09-30T02:42:09.040106+08:00. ReadyTrue; transport readyTrue; loop healthyTrue. ModeHype; output253/255; emissionTrue; revision2160. Accepted1073810140, rendered1073810140, emitted1073810140. Pending fields[]; pending actions0.

## Operator state

Reconnect resets soft pickup. A full low-to-high sweep of Fader7 crosses the current saturation target and reacquires control; subsequent movement applies normally. Do not bypass pickup or send default values to make a status indicator green. Fader1/Fader4 retain the separate previously documented identity-qualification hold. Their gate was not the cause of the all-fader outage and was not removed in this recovery.

Evidence in this directory preserves stale pre-recovery state, launch/process identity, live status/target observations and the unmodified admission copy. Broker console output is retained in broker-console.log. Exact prior termination cause and optical judgement are not established.
