"""Classify quiet → hit → quiet-gap darkness. Near-black is not black."""
from __future__ import annotations
import math

from emit_continuity import emit_continuity_ok

TRUE_BLACK = 'TRUE_BLACK'
RESIDUAL_HISTORY = 'RESIDUAL_HISTORY'
FRESH_SOUND = 'FRESH_SOUND'
INCONCLUSIVE_ROOM = 'INCONCLUSIVE_ROOM'
INCONCLUSIVE_HIT = 'INCONCLUSIVE_HIT'
FAIL_EMIT = 'FAIL_EMIT'
INCONCLUSIVE_DATA = 'INCONCLUSIVE_DATA'

QUIET_HOP_ABS = 1500
QUIET_HOLD_S = 2.0
SILENCE_HOLD_S = 5.0
HIT_WINDOW_S = 1.5
MAX_SAMPLE_GAP_S = 0.5
FRAME_BYTES = 160 * 3


def capture_error(samples, hit_at_s, max_sample_gap_s):
    """Validate evidence before any missing value can be interpreted as black."""
    if not isinstance(samples, (list, tuple)) or len(samples) < 2:
        return 'need at least two complete samples'
    if (isinstance(hit_at_s, bool) or not isinstance(hit_at_s, (int, float))
            or not math.isfinite(hit_at_s)):
        return 'invalid hit timestamp'
    previous_t = None
    bounds = {'last_hop_peak': 32768, 'led_max': 255,
              'led_nz': FRAME_BYTES, 'emitted': None, 'emit_errors': None,
              'frame_bytes': FRAME_BYTES}
    for i, s in enumerate(samples):
        if not isinstance(s, dict):
            return f'sample {i}: expected an object'
        t = s.get('t')
        if (isinstance(t, bool) or not isinstance(t, (int, float))
                or not math.isfinite(t) or t < 0):
            return f'sample {i}: invalid timestamp'
        if previous_t is not None and not 0 < t - previous_t <= max_sample_gap_s:
            return f'sample {i}: timestamp reversed/duplicated or observation gap'
        previous_t = t
        for key, upper in bounds.items():
            value = s.get(key)
            if (isinstance(value, bool) or not isinstance(value, int)
                    or value < 0 or (upper is not None and value > upper)):
                return f'sample {i}: missing or invalid {key}'
        if s['frame_bytes'] != FRAME_BYTES:
            return f'sample {i}: incomplete native RGB frame'
        if (s['led_max'] == 0) != (s['led_nz'] == 0):
            return f'sample {i}: inconsistent black summary'
    if not samples[0]['t'] <= hit_at_s <= samples[-1]['t']:
        return 'hit lies outside the capture'
    return None


def stats(rows, key):
    vals = [s[key] for s in rows if s.get(key) is not None]
    if not vals:
        return None
    vals = sorted(vals)
    return {'n': len(vals), 'min': vals[0], 'med': vals[len(vals) // 2],
            'max': vals[-1]}


def first_hold(samples, pred, hold_s, t_key='t',
               max_sample_gap_s=MAX_SAMPLE_GAP_S):
    if not samples:
        return None
    start = None
    previous_t = None
    for s in samples:
        if previous_t is not None and not 0 < s[t_key] - previous_t <= max_sample_gap_s:
            start = None
        previous_t = s[t_key]
        if pred(s):
            if start is None:
                start = s
            if s[t_key] - start[t_key] >= hold_s:
                return start[t_key], s[t_key]
        else:
            start = None
    return None


def last_hold_tail(samples, pred, hold_s, t_key='t',
                   max_sample_gap_s=MAX_SAMPLE_GAP_S):
    """Last contiguous run of pred, scored on its final hold_s seconds."""
    if not samples:
        return None
    best = None
    start = None
    last = None
    previous_t = None
    for s in samples:
        if previous_t is not None and not 0 < s[t_key] - previous_t <= max_sample_gap_s:
            start = None
            last = None
        previous_t = s[t_key]
        if pred(s):
            if start is None:
                start = s
            last = s
        else:
            if start is not None and last is not None and last[t_key] - start[t_key] >= hold_s:
                best = (start, last)
            start = None
            last = None
    if start is not None and last is not None and last[t_key] - start[t_key] >= hold_s:
        best = (start, last)
    if best is None:
        return None
    _, last = best
    return last[t_key] - hold_s, last[t_key]


def hop_floor(quiet_max):
    return max(int(quiet_max * 1.5), int(quiet_max) + 300, QUIET_HOP_ABS)


def classify(samples, hit_at_s, quiet_hop_abs=QUIET_HOP_ABS,
             quiet_hold_s=QUIET_HOLD_S, silence_hold_s=SILENCE_HOLD_S,
             max_stall_s=2.0, max_sample_gap_s=MAX_SAMPLE_GAP_S):
    for value in (quiet_hop_abs, quiet_hold_s, silence_hold_s, max_stall_s,
                  max_sample_gap_s):
        if (isinstance(value, bool) or not isinstance(value, (int, float))
                or not math.isfinite(value) or value <= 0):
            raise ValueError('acceptance thresholds must be finite and positive')
    error = capture_error(samples, hit_at_s, max_sample_gap_s)
    if error:
        return {'cause': INCONCLUSIVE_DATA, 'reason': error,
                'evidence_valid': False, 'emit_ok': False,
                'max_sample_gap_s': max_sample_gap_s}
    if any(s['emit_errors'] for s in samples):
        return {'cause': FAIL_EMIT, 'reason': 'nonzero emit error counter',
                'evidence_valid': True, 'emit_ok': False}
    if any(b['emitted'] < a['emitted'] for a, b in zip(samples, samples[1:])):
        return {'cause': FAIL_EMIT, 'reason': 'emit counter reset/reversed',
                'evidence_valid': True, 'emit_ok': False}
    emit_ok, emit_stall_s, emit_stall_at, emit_stall_val = emit_continuity_ok(
        samples, max_stall_s=max_stall_s)
    before = [s for s in samples if s['t'] <= hit_at_s]
    after = [s for s in samples if s['t'] >= hit_at_s]
    quiet_hold = first_hold(
        before, lambda s: (s.get('last_hop_peak') or 0) <= quiet_hop_abs,
        quiet_hold_s, max_sample_gap_s=max_sample_gap_s)
    late_near_black = bool(after) and all(
        (s.get('led_max') or 0) <= 1 for s in after[-6:])
    true_black_tail = bool(after) and all(
        (s.get('led_nz') or 0) == 0 for s in after[-6:])
    result = {
        'evidence_valid': True,
        'max_sample_gap_s': max_sample_gap_s,
        'emit_ok': emit_ok,
        'emit_stall_s': emit_stall_s,
        'emit_stall_at_s': emit_stall_at,
        'emit_stall_val': emit_stall_val,
        'late_near_black': late_near_black,
        'true_black_tail': true_black_tail,
        'quiet_hold': quiet_hold,
        'quiet_hop_abs': quiet_hop_abs,
    }
    last_draw_t = None
    last_effect = None
    for s in after:
        if s.get('musical') or s.get('visual_path') == 'effect':
            last_draw_t = s['t']
        effect = s.get('effect_frames')
        if last_effect is not None and effect is not None and effect > last_effect:
            last_draw_t = s['t']
        if effect is not None:
            last_effect = effect
    seen_lit = False
    first_zero = None
    for s in after:
        if (s.get('led_nz') or 0) > 0:
            seen_lit = True
        elif seen_lit:
            first_zero = s
            break
    first_zero_t = None if first_zero is None else first_zero['t']
    live_age_s = None
    if first_zero is not None and first_zero.get('live_age_us'):
        live_age_s = round(first_zero['live_age_us'] / 1e6, 3)
    fade_s = live_age_s
    if fade_s is None and last_draw_t is not None and first_zero_t is not None:
        fade_s = round(first_zero_t - last_draw_t, 3)
    black_hold = last_hold_tail(
        after, lambda s: (s.get('led_nz') or 0) == 0, silence_hold_s)
    result.update(
        last_draw_t=last_draw_t,
        first_zero_t=first_zero_t,
        fade_s=fade_s,
        fade_source=('live_age_us' if live_age_s is not None else 'sample_delta'),
        black_hold=black_hold,
        black_hold_s=(None if black_hold is None
                      else round(black_hold[1] - black_hold[0], 3)))
    if not emit_ok:
        result.update(cause=FAIL_EMIT,
                      reason=f'emit stalled {emit_stall_s:.3f}s at t={emit_stall_at}')
        return result
    if quiet_hold is None:
        result.update(
            cause=INCONCLUSIVE_ROOM,
            reason=f'no {quiet_hold_s:.1f}s hop<={quiet_hop_abs} before the hit',
            baseline_hop_peak=stats(before, 'last_hop_peak'),
            baseline_led_max=stats(before, 'led_max'),
            baseline_led_nz=stats(before, 'led_nz'))
        return result
    q0, q1 = quiet_hold
    quiet_rows = [s for s in before if q0 <= s['t'] <= q1]
    established_black = first_hold(
        quiet_rows, lambda s: s['led_nz'] == 0, min(1.0, quiet_hold_s),
        max_sample_gap_s=max_sample_gap_s)
    result['established_black'] = established_black
    baseline_peak = stats(quiet_rows, 'last_hop_peak') or {'med': 0, 'max': 0}
    baseline_led = stats(quiet_rows, 'led_max') or {'med': 0, 'max': 0}
    baseline_nz = stats(quiet_rows, 'led_nz') or {'med': 0, 'max': 0}
    quiet_max = baseline_peak.get('max') or 0
    floor = max(hop_floor(quiet_max), quiet_hop_abs)
    hit_window = [s for s in after if s['t'] < hit_at_s + HIT_WINDOW_S]
    hit_peak = stats(hit_window, 'last_hop_peak') or {'max': 0}
    hit_led = stats(hit_window, 'led_max') or {'max': 0}
    pcm_floor = max(3 * (baseline_peak.get('med') or 0), quiet_max + 500)
    led_floor = max((baseline_led.get('max') or 0) + 8, 16)
    event_pcm = (hit_peak.get('max') or 0) >= pcm_floor
    event_led = (hit_led.get('max') or 0) >= led_floor
    result.update(
        quiet_rows=len(quiet_rows),
        baseline_hop_peak=baseline_peak,
        baseline_led_max=baseline_led,
        baseline_led_nz=baseline_nz,
        hop_floor=floor,
        hit_hop_peak=hit_peak,
        hit_led_max=hit_led,
        pcm_margin_required=pcm_floor,
        led_margin_required=led_floor,
        event_pcm=event_pcm,
        event_led=event_led,
        distinguishable=event_pcm and event_led)
    if not (event_pcm and event_led):
        result.update(
            cause=INCONCLUSIVE_HIT,
            reason='hit did not exceed predeclared PCM/LED margin over qualified quiet')
        return result
    eligible = [s for s in after if s['t'] >= hit_at_s + HIT_WINDOW_S]
    silence_hold = last_hold_tail(
        eligible, lambda s: s['last_hop_peak'] <= floor, silence_hold_s,
        max_sample_gap_s=max_sample_gap_s)
    qualified_gap_s = silence_hold_s if silence_hold is not None else None
    black_hold = last_hold_tail(
        eligible, lambda s: s['last_hop_peak'] <= floor and s['led_nz'] == 0,
        silence_hold_s, max_sample_gap_s=max_sample_gap_s)
    result.update(black_hold=black_hold,
                  black_hold_s=(None if black_hold is None
                                else round(black_hold[1] - black_hold[0], 3)))
    result['silence_hold'] = silence_hold
    result['qualified_gap_s'] = qualified_gap_s
    if silence_hold is None:
        after_peak = stats(eligible or after, 'last_hop_peak')
        fresh_hold = first_hold(
            eligible, lambda s: s['last_hop_peak'] > floor, silence_hold_s,
            max_sample_gap_s=max_sample_gap_s)
        result.update(
            cause=FRESH_SOUND if fresh_hold else INCONCLUSIVE_DATA,
            reason=(f'hop stayed above floor {floor} for {silence_hold_s:.1f}s'
                    if fresh_hold else
                    f'no observed {silence_hold_s:.1f}s quiet gap after hit'),
            after_hop_peak=after_peak)
        return result
    s0, s1 = silence_hold
    silence_rows = [s for s in after if s0 <= s['t'] <= s1]
    nz = stats(silence_rows, 'led_nz') or {'max': 0}
    mx = stats(silence_rows, 'led_max') or {'max': 0}
    result.update(silence_led_nz=nz, silence_led_max=mx,
                  silence_hop_peak=stats(silence_rows, 'last_hop_peak'))
    if established_black is None:
        result.update(
            cause=INCONCLUSIVE_ROOM,
            reason='pre-hit quiet was not established black')
        return result
    if black_hold is not None and black_hold[1] == samples[-1]['t']:
        result.update(cause=TRUE_BLACK,
                      reason='established black, distinguishable hit, '
                             'declared quiet-and-exact-black tail')
        return result
    if samples[-1]['last_hop_peak'] > floor:
        result.update(cause=FRESH_SOUND,
                      reason='sound resumed after the qualified quiet interval')
        return result
    if (nz.get('max') or 0) == 0:
        result.update(cause=INCONCLUSIVE_DATA,
                      reason='black interval was not sustained through capture end')
        return result
    result.update(
        cause=RESIDUAL_HISTORY,
        reason='qualified quiet gap at hop floor, Pixel8 still occupied')
    return result
