# Captain's bench card — SMC-Mixer → Titan

## What you do first

Connect the SMC-Mixer to the Mac by USB, if it is not already connected. Keep Titan and the LED wiring unchanged. Do not reset, pair, reprogramme or move a wire.

**Wait for Grok to say capture is recording before moving controls.** There are two different “ready” calls: mixer input only, then the real board. Input-only feedback is not yet Titan control.

## 1. Mixer-only check

Grok says **MIXER INPUT RECORDING — NO TITAN WRITES**.

Keep still for five seconds. Move fader 7 slowly bottom → top → halfway, then stop. Afterwards, move fader 6 a little when asked. This identifies the actual control and checks that the neighbour is not mistaken for it. No Titan command is sent.

## 2. One board decision

Grok supplies one verified firmware candidate, exact HEX hash, scope and failure policy. You should not be asked to choose among unexplained hashes.

The proposed approval phrase is **GO INSTALL + EMIT-OFF**, only after those details are supplied. It covers the named installation, startup/identity checks and the one-field control test. It does not enable light.

If a known software/candidate defect still blocks the board, Grok must say exactly what it is and stop asking you to move controls while repairing it.

## 3. Real fader check

Grok says **LIVE BOARD CAPTURE RUNNING — EMISSION OFF**, names the verified build and displays the actual current motion value/pickup position.

Keep fader 7 still initially. Move it slowly across pickup when prompted. Then try approximately quarter → three-quarter → halfway travel, pausing at each point. Grok checks actual requested/accepted/rendered values; you do not need to hit exact numbers.

Move fader 6 a little when prompted; it must not change the scoped target parameter.

For the reconnect check, unplug/replug **only the mixer USB cable**. Leave fader 7 parked. The engine must not jump to that parked position just because the mixer reconnects. Reacquire pickup when prompted.

## 4. Look at the light

Grok supplies the actual proposed output limit, light-test window and final output state. The proposed approval phrase is **GO LIGHT** for that named scope. Any one-time scene-start reset must be disclosed first.

With the plate visible and capture running, move fader 7 lower and higher, then make small adjustments. Report what you actually see: slower/faster, unchanged, jumps, resets, sparkles, dropouts; and whether the response feels useful or sluggish.

**Your observation is part of this test. A software dump does not replace it.** Emission-off testing cannot prove light, and an apparently fast response is not a calibrated latency measurement.

## Leave-state

The agreed light-test end state must be explicit: output off, or the approved scene left running. No automatic restoration of the old emitting `980eb3a9…` firmware. No surprise reflash or wire change.
