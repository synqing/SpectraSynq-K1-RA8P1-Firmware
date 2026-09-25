# E2 Lite macOS: verified stale host-lock recovery

Investigation date: 22 September 2026, AWST. Probe: E2L: 5AS079228B.

## Outcome

The persistent host-side in-use flag was found and removed. Renesas's own library subsequently completed three exclusive USB open/close cycles and released its lock correctly after each cycle. This is not merely RFP enumeration.

**The Titan RA8P1 SWD/DAP connection is NOT yet verified. No target attach was attempted during this investigation.** Do not mark CPU0 debugging complete or infer that the earlier 0x00030815 error is solved.

## Evidence

The installed ARM/E2_v2.8.1/libCommuni.dylib constructs a POSIX semaphore name from the probe USB serial:

    /CommuniDLL_SemE2L: 5AS079228B

Its discovery routine tests whether that named object exists and sets the emulator-in-use result true when it does. Its normal close routine calls sem_close and sem_unlink. The native semaphore existed with no matching debugger processes and no consumers of the inspected communication library.

Before removal, COM_GetSerial returned the exact probe and in-use=true. A guarded sem_unlink removed ONLY that name; the next three COM_GetSerial calls returned in-use=false. Three COM_Open/COM_Close cycles then returned1 for both operations, with the semaphore present during ownership and absent after closing. The final read-only guard returned HOST_LOCK_CLEAR.

These are bounded observations from this installed library, not a claim about every Renesas release. They establish the abandoned host-lock mechanism. The precise earlier process that first abandoned it cannot be identified from the supplied history. Forced server terminations in that history are consistent with the missing normal cleanup.

The Mac was not rebooted, the probe USB was not reset, and no tool firmware update was performed. No MCU erase, download, authentication-level change, power-supply setting, reset command, or target transaction was performed.

## Files

- host-lock-removal.json: actual before/after semaphore receipt and two idle-owner checks.
- baseline-enumeration-observation.json: explicitly labelled transcription of the captured pre-cleanup native discovery result.
- post-cleanup-enumeration.json: actual three-run Renesas discovery result.
- usb-open-close-validation.json: actual three-cycle exclusive USB ownership validation.
- final-host-check.json: actual installed recovery helper's final read-only result.
- runtime-metadata.json: OS, runtime path, and binary identities.
- static-lock-evidence.txt: selected native instructions establishing the mechanism.
- e2lite_lock_guard.py: checked, hash-pinned, inspect-by-default recovery helper.
- sources.json: primary documentation and first-hand community report examined.

## Reuse

The folder is already installed on the Mac at:

    /Users/spectrasynq/Applications/Renesas/E2_Debug_Runtime_10.6.0/diagnostics/20260922-stale-semaphore

Inspect without creating or removing anything:

    python3 e2lite_lock_guard.py

Only after all debugger/programmer clients have been closed normally, recover a recurrence:

    python3 e2lite_lock_guard.py --clear-stale

The helper does not kill processes, claim USB, reset hardware, or invoke GDB. It refuses cleanup when its active-owner checks find a possible consumer or cannot be completed. It also refuses an unreviewed communication-library version. It never overwrites a requested receipt file. Default exit2 means the named lock is present, not automatically that it is stale; exit0 means no named lock remains. Exit1 means STOP/error.

Keep all other clients stopped throughout cleanup. Process checks cannot prevent another application launching immediately afterwards. Never treat this helper as permission to remove a genuine active debugger's lock. Never generalize its one exact sem_unlink into bulk IPC deletion.

## Resuming target debugging

Preserve the full known runtime and the resident image. Use R7KA8P1KF, CPU0, SWD, emulator power OFF; do not restore the old RA0E1 launch or enable downloads/security writes. The supplied history recorded J6 disconnected; verify the physical state before any target attach. A target attach, including hotplug, is a separate action and can change run state.

Do not repeat the previous clock sweep or assume that clearing the host lock repairs wiring. After physical inspection, use one controlled attempt and classify its actual error. A returned0x00030815 is the older DAP-layer issue, not evidence that the host-lock fix failed.

Official wiring references are S6 Figure2.3 and connector tables. Ground-before-hotplug requirements are S5 section2.4/Figure2.5. For RA8 hotplug, follow S6 section3.3.20 including authentication None; do not fit the ribbon before the prescribed prompt. The RA4M2 forum boundary-setting workaround S8 is not a reason to modify Titan's TrustZone or DLM state.

Completion requires a real CPU0 connection and register read followed by orderly detach and repeatable reconnect. None of those target-level criteria is claimed here.

## Prevention and upstream report

Use one debugger owner. Prefer an orderly disconnect/close over terminating the server. After a crash or forced termination, inspect the named flag before retrying. Do not automatically rerun GDB on the same ownership error.

Report to Renesas: macOS26.1 arm64, server10.6.0.v20260707-035422, communication component2.8.1, exact library hashes in runtime-metadata.json; orphaned POSIX name causes discovery to report busy without a live owner; unlinking the exact abandoned name restores discovery and exclusive USB open/close. Request lifecycle cleanup/orphan detection. This is a locally verified workaround, not a published vendor patch.
