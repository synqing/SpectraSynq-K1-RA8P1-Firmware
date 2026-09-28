"""Independent direct complex-correlation oracle. No firmware implementation imports."""
import cmath
import math
from fractions import Fraction

N = 512
INTERVAL_FRAMES = 1080
MEDIA_RATE = 48000
UNIT_ROUNDOFF = 2.0 ** -53


def circular_error(a, b):
    return abs((a - b + 0.5) % 1.0 - 0.5)


def correlate(samples, bpm, interval=INTERVAL_FRAMES):
    if len(samples) != N or not math.isfinite(bpm) or bpm <= 0 or interval <= 0:
        return None
    if any(not math.isfinite(x) or x < 0 for x in samples):
        return None
    # Chronological negative sample times; complex phasor sign is explicit.
    terms = [x * cmath.exp(-2j * math.pi * (bpm / 60) *
                          ((i - N + 1) * interval / MEDIA_RATE))
             for i, x in enumerate(samples)]
    z = complex(math.fsum(t.real for t in terms), math.fsum(t.imag for t in terms))
    l1 = math.fsum(abs(x) for x in samples)
    angle_max = 2 * math.pi * (bpm / 60) * ((N - 1) * interval / MEDIA_RATE)
    gamma = (8 * N * UNIT_ROUNDOFF) / (1 - 8 * N * UNIT_ROUNDOFF)
    vector_bound = l1 * (gamma + 8 * UNIT_ROUNDOFF * (1 + angle_max))
    if abs(z) <= 2 * vector_bound or abs(z) == 0:
        return None
    # Includes error of both direct computations, transcendental/angle formation,
    # atan2 final rounding; output float quantisation is added by the caller.
    tolerance = math.asin(min(1, 2 * vector_bound / abs(z))) / (2 * math.pi) + 8 * UNIT_ROUNDOFF
    return {'phase': (cmath.phase(z) / (2 * math.pi)) % 1,
            'tolerance_cycles': tolerance, 'magnitude': abs(z), 'l1': l1}


def impulse_phase(position, bpm, interval=INTERVAL_FRAMES):
    return (Fraction(str(bpm)) * (N - 1 - position) * interval / (60 * MEDIA_RATE)) % 1


def derived_phase(beat_index, phase, numerator, denominator):
    if denominator <= 0 or numerator <= 0:
        return None
    return ((Fraction(beat_index) + Fraction(phase)) * numerator / denominator) % 1
