# Capture protocol guards

Titan continues autonomous music visualisation when an observation ends. These
host checks do not grant target, camera, playback or service authority.

From this repository, run the tracked, device-free verification:

```sh
python scripts/test_capture_protocol.py
```

To also inspect the retained session callers and evaluate their actual warm-child
argv without running them:

```sh
python scripts/test_capture_protocol.py --consumers _scratch/visible-dual-20261011
```

The second command requires that existing directory; it does not create another
checkout or launch any recorder, player, broker or probe. It checks the current
recorder, direct executor and warm caller syntax, required guard calls and actual
warm-child argv expression. It evaluates only that expression and its pure bound
assignment. Full service, camera and target execution are deliberately excluded.

`scripts/capture_protocol.py` is an importable standard-library module. The
retained session `record_phase.py`, `execute_direct_phase.py` and
`warm_start_phase.py` consume it. The scratch orchestrators are session-specific
tools, not a general supported release launcher. Historical programme helpers
remain historical and do not acquire these guards automatically.

## Enforced predicates

- Measured caller argv must equal the selected plan's recording, playback and
  wall durations. Finite values, local caps and remaining outer time are checked.
- The phase deadline is translated once from the retained outer epoch into a
  monotonic deadline, with the recovery reserve subtracted.
- Warm child time is bounded by both the original outer cutoff and shared warm
  cutoff, including its full setup/capture/cleanup requirement. The warm caller
  requires `start_utc`, `outer_seconds`, `outer_deadline_utc`, `warm_start_utc`
  and `warm_deadline_utc`; the latter must be first-warm time plus 300 seconds.
  Missing provenance refuses before service or probe activity.
- Existing evidence paths, including dangling symlinks, are refused. Directory
  creation also refuses races. Failed evidence is never recycled as fresh output.
- Owner hello admission must match the plan UID, exact build and schema. Related
  source or an older successful clip cannot stand in for this image.
- A control stage must have a running capture child and remain before the fixed
  phase/outer cutoff. A successful child return alone is insufficient: exact
  delegate start and finish timestamps must cover the whole requested duration,
  with no camera error, cancellation, duplicate or missing boundary.
- Process ownership matching uses executable argv structure, avoiding scanner
  strings inside `python -c`. It recognises the current unflagged Python mixer
  and broker forms and exact camera/player forms; new invocation forms must be
  reviewed rather than silently broadened.

## Limits that remain procedural

The checks cannot protect a retained clock file from an authorised caller
rewriting its original anchor or inventing a new file. Preserve the first anchor,
failure, attempts and external authority. Never renew a running window.

Reserve setup, actual camera-start delay, full capture and finalisation before
launch. The Swift helper additionally refuses when actual `didStart` leaves too
little time for the full duration and four-second finalisation reserve; it does
not shorten a recording into success. The guards cannot predict OS preparation
latency. The guard module does not globally bound blocking RPCs, service commands,
cleanup or historical programming helpers. Report any deadline overrun honestly.

Focus acquisition before conditional locking is source policy, not proof of a
usable original picture. Open the actual original frame and whole-video evidence
as required. An error-free playable container can still be blurred, incomplete,
camera-clipped or unsuitable. Provider disconnect logs and owned cancellation
are different causes; preserve both. Never infer an LED transport fault from MOV
edit lists, frame-rate conversion or camera exposure alone.

No test here establishes optical quality, music response, current device identity,
electrical capacity, completed physical acceptance or safe programming. The
canonical deployment gateway and its independent UID/probe-power/range checks
remain the only authorised programming route.

Session-specific causal evidence and preserved baseline files are at
`_scratch/visible-dual-20261011/lesson-canon-20261011/capture.md`. The canonical
cross-project session lessons remain in Kx-Titan's migration documentation.
