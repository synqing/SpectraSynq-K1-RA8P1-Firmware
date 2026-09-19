"""Fail a sustained emit stall while the renderer is enabled."""


def longest_emit_stall_s(samples, t_key='t', emit_key='emitted'):
    if len(samples) < 2:
        return 0.0, None, None
    start = samples[0]
    longest = 0.0
    longest_at = samples[0][t_key]
    longest_val = samples[0].get(emit_key)
    for i in range(1, len(samples)):
        prev, cur = samples[i - 1], samples[i]
        if cur.get(emit_key) != start.get(emit_key):
            start = cur
            continue
        dt = cur[t_key] - start[t_key]
        if dt > longest:
            longest = dt
            longest_at = start[t_key]
            longest_val = start.get(emit_key)
    return longest, longest_at, longest_val


def emit_continuity_ok(samples, max_stall_s=2.0):
    longest, at, val = longest_emit_stall_s(samples)
    return longest < max_stall_s, longest, at, val
