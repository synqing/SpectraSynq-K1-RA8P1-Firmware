"""Pure admission checks for bounded evidence capture; no device or process I/O.

Titan normal music output is autonomous. A capture cutoff ends observation only.
These checks grant no hardware authority and make no optical-quality claim.
"""
import datetime
import math
import os
import shlex


def capture_bounds(phase, declared_record, declared_playback, declared_wall,
                   outer_deadline, recovery_reserve, now_epoch, now_monotonic):
    values = (declared_record, declared_playback, declared_wall, outer_deadline,
              recovery_reserve, now_epoch, now_monotonic)
    if not all(math.isfinite(x) for x in values):
        raise ValueError('Non-finite capture bounds')
    if (float(phase['record_seconds']) != declared_record or
        float(phase.get('playback_seconds', phase['record_seconds'])) != declared_playback or
        float(phase['wall_seconds']) != declared_wall):
        raise ValueError('Declared media duration differs from selected phase')
    if not (0 < declared_playback <= declared_record <= 112 and
            declared_record + 8 <= declared_wall <= 120 and recovery_reserve >= 60):
        raise ValueError('Invalid media/local/recovery bounds')
    remaining = outer_deadline - recovery_reserve - now_epoch
    if remaining < declared_wall:
        raise ValueError('Insufficient fixed outer time before recovery reserve')
    return min(now_monotonic + declared_wall, now_monotonic + remaining)


def anchored_deadline(start_epoch, duration, recorded_deadline, declared_deadline):
    """Reject renewal against the caller's retained anchor, not just remaining time."""
    if not all(math.isfinite(x) for x in (start_epoch, duration, recorded_deadline, declared_deadline)):
        raise ValueError('Non-finite window')
    if duration <= 0 or abs(start_epoch + duration - recorded_deadline) > .001:
        raise ValueError('Window deadline differs from original anchor')
    if recorded_deadline != declared_deadline:
        raise ValueError('Caller renewed the fixed deadline')
    return recorded_deadline


def warm_child_outer(outer_deadline, warm_deadline, reserve, now_epoch, required_wall=80):
    if not all(math.isfinite(x) for x in (outer_deadline, warm_deadline, reserve, now_epoch, required_wall)):
        raise ValueError('Non-finite warm bounds')
    if reserve < 60 or required_wall <= 0:
        raise ValueError('Invalid warm reserve')
    stop = min(outer_deadline - reserve, warm_deadline)
    if now_epoch + required_wall > stop:
        raise ValueError('Insufficient shared warm time for full child and cleanup')
    return stop + reserve


def require_fresh_output(path):
    # lexists also catches dangling symlinks; never replace failed evidence.
    if os.path.lexists(path):
        raise ValueError('Evidence output is single use')


def require_active(now_epoch, fixed_stop_epoch, child_returncode):
    if (not math.isfinite(now_epoch) or not math.isfinite(fixed_stop_epoch) or
        now_epoch >= fixed_stop_epoch or child_returncode is not None):
        raise ValueError('Capture ended or its fixed activity cutoff was reached')


def require_identity(candidate, hello):
    bound = hello.get('bound', {})
    if not hello.get('ok') or not hello.get('admitted'):
        raise ValueError('Current owner admission required')
    for wanted, observed in (('uid', 'uid'), ('build_id', 'build'), ('schema_sha256', 'schema_sha256')):
        if not candidate.get(wanted) or candidate[wanted] != bound.get(observed):
            raise ValueError('Current device identity differs: ' + wanted)


def require_recording_complete(events, expected_seconds, returncode, interrupted=False):
    """A playable partial container and rc0 alone are insufficient evidence."""
    if not math.isfinite(expected_seconds) or expected_seconds <= 0:
        raise ValueError('Invalid expected duration')
    starts = [e for e in events if e.get('event') == 'recording_started']
    ends = [e for e in events if e.get('event') == 'recording_finished']
    if returncode != 0 or interrupted or len(starts) != 1 or len(ends) != 1 or ends[0].get('error'):
        raise ValueError('Recording failed, interrupted or lacks exact start/finish')
    def epoch(event):
        stamp = datetime.datetime.fromisoformat(event['utc'].replace('Z', '+00:00'))
        if stamp.tzinfo is None:
            raise ValueError('Recording timestamp lacks timezone')
        return stamp.timestamp()
    # Delegate timestamps delimit recording, not the helper's preparation period.
    if epoch(ends[0]) - epoch(starts[0]) < expected_seconds:
        raise ValueError('Recording finished before its full requested duration')


def owned_command(command, kind):
    """Match executable argv structure, never text mentioned inside python -c."""
    try:
        args = shlex.split(command)
    except ValueError:
        return False
    if not args:
        return False
    executable = os.path.basename(args[0])
    python = executable.lower().startswith('python')
    if kind == 'mixer':
        return python and args[1:4] == ['-m', 'smc_titan_control', 'live']
    if kind == 'broker':
        return python and len(args) >= 3 and os.path.basename(args[1]) == 'titan_broker.py' and args[2] in ('daemon', 'serve')
    if kind == 'camera':
        return executable == 'product_camera' and args[1:2] == ['capture']
    if kind == 'player':
        return args[0] == '/usr/bin/afplay' and len(args) == 2
    raise ValueError('Unknown owned process kind')
