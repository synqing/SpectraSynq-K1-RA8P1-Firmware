"""Near-black is not black. Hop floor decides leftover picture vs fresh sound."""
import importlib.util
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
spec = importlib.util.spec_from_file_location(
    'quiet_gap_score', ROOT / 'scripts/quiet_gap_score.py')
score = importlib.util.module_from_spec(spec)
spec.loader.exec_module(score)


def row(t, hop, led_max, led_nz, emitted=None):
    if emitted is None:
        emitted = int(t * 120)
    return {
        't': t, 'last_hop_peak': hop, 'led_max': led_max, 'led_nz': led_nz,
        'emitted': emitted,
        'emit_errors': 0, 'frame_bytes': 480,
    }


def series(spec_rows):
    out = []
    t = 0.0
    emitted = 0
    for n, hop, led_max, led_nz, dt in spec_rows:
        for _ in range(n):
            emitted += 1
            out.append(row(round(t, 3), hop, led_max, led_nz, emitted))
            t += dt
    return out


class QuietGapScore(unittest.TestCase):
    def valid_capture(self):
        return series([
            (50, 200, 0, 0, 0.05),
            (10, 8000, 80, 40, 0.05),
            (20, 400, 20, 10, 0.05),
            (120, 220, 0, 0, 0.05),
        ])

    def test_missing_required_evidence_never_passes(self):
        for key in ('t', 'last_hop_peak', 'led_max', 'led_nz',
                    'emitted', 'emit_errors', 'frame_bytes'):
            with self.subTest(key=key):
                samples = self.valid_capture()
                for s in samples[80:]:
                    s.pop(key)
                self.assertEqual(score.classify(samples, 2.5)['cause'],
                                 'INCONCLUSIVE_DATA')

    def test_malformed_evidence_never_passes(self):
        cases = [('t', float('nan')), ('t', float('inf')), ('t', '4'),
                 ('last_hop_peak', None), ('last_hop_peak', -1),
                 ('led_max', float('nan')), ('led_max', 256),
                 ('led_nz', -1), ('led_nz', 481), ('led_nz', True),
                 ('emitted', None), ('frame_bytes', 479)]
        for key, value in cases:
            with self.subTest(key=key, value=value):
                samples = self.valid_capture()
                samples[100][key] = value
                self.assertEqual(score.classify(samples, 2.5)['cause'],
                                 'INCONCLUSIVE_DATA')

    def test_inconsistent_black_summary_never_passes(self):
        samples = self.valid_capture()
        samples[-1]['led_max'] = 12
        self.assertEqual(score.classify(samples, 2.5)['cause'],
                         'INCONCLUSIVE_DATA')

    def test_capture_gap_is_not_sustained_black(self):
        samples = self.valid_capture()
        samples = samples[:85] + samples[-2:]
        self.assertEqual(score.classify(samples, 2.5)['cause'],
                         'INCONCLUSIVE_DATA')

    def test_duplicate_or_reversed_time_never_passes(self):
        for delta in (0, -0.05):
            with self.subTest(delta=delta):
                samples = self.valid_capture()
                samples[100]['t'] = samples[99]['t'] + delta
                self.assertEqual(score.classify(samples, 2.5)['cause'],
                                 'INCONCLUSIVE_DATA')

    def test_empty_or_single_sample_never_passes(self):
        for samples in ([], [row(0, 200, 0, 0)]):
            with self.subTest(samples=samples):
                self.assertEqual(score.classify(samples, 2.5)['cause'],
                                 'INCONCLUSIVE_DATA')

    def test_emit_counter_reset_fails(self):
        samples = self.valid_capture()
        samples[100]['emitted'] = 0
        self.assertEqual(score.classify(samples, 2.5)['cause'], score.FAIL_EMIT)

    def test_emit_error_fails_even_if_counter_advances(self):
        samples = self.valid_capture()
        samples[-1]['emit_errors'] = 1
        self.assertEqual(score.classify(samples, 2.5)['cause'], score.FAIL_EMIT)

    def test_three_seconds_cannot_replace_declared_five(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (30, 8000, 80, 40, 0.05),
            (65, 220, 0, 0, 0.05),
        ])
        got = score.classify(samples, 2.5)
        self.assertEqual(got['cause'], 'INCONCLUSIVE_DATA')
        self.assertIsNone(got['qualified_gap_s'])

    def test_black_and_quiet_must_overlap_for_full_hold(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (30, 8000, 80, 40, 0.05),
            (120, 6000, 0, 0, 0.05),
            (120, 220, 20, 10, 0.05),
        ])
        self.assertEqual(score.classify(samples, 2.5)['cause'],
                         score.RESIDUAL_HISTORY)

    def test_short_exact_black_tail_does_not_pass(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (30, 8000, 80, 40, 0.05),
            (120, 220, 20, 10, 0.05),
            (65, 220, 0, 0, 0.05),
        ])
        self.assertNotEqual(score.classify(samples, 2.5)['cause'], score.TRUE_BLACK)

    def test_reillumination_after_black_hold_does_not_pass(self):
        samples = self.valid_capture()
        samples.append(row(10.0, 220, 20, 10, samples[-1]['emitted'] + 1))
        self.assertNotEqual(score.classify(samples, 2.5)['cause'], score.TRUE_BLACK)

    def test_hold_helpers_cannot_bridge_an_observation_gap(self):
        samples = [row(0, 200, 0, 0), row(6, 200, 0, 0)]
        self.assertIsNone(score.first_hold(samples, lambda s: True, 2.0))
        self.assertIsNone(score.last_hold_tail(samples, lambda s: True, 5.0))

    def test_acceptance_thresholds_are_explicit_and_valid(self):
        for key in ('quiet_hold_s', 'silence_hold_s', 'max_stall_s',
                    'max_sample_gap_s'):
            for value in (0, -1, float('nan'), float('inf'), True):
                with self.subTest(key=key, value=value):
                    with self.assertRaises(ValueError):
                        score.classify(self.valid_capture(), 2.5, **{key: value})

    def test_invalid_or_out_of_capture_hit_is_inconclusive(self):
        for hit in (None, float('nan'), float('inf'), -1, 100):
            with self.subTest(hit=hit):
                self.assertEqual(score.classify(self.valid_capture(), hit)['cause'],
                                 'INCONCLUSIVE_DATA')

    def test_near_black_with_hop_floor_is_history_not_pass(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (10, 8000, 80, 40, 0.05),
            (20, 300, 40, 20, 0.05),
            (120, 250, 1, 90, 0.05),
        ])
        got = score.classify(samples, hit_at_s=2.5)
        self.assertEqual(got['cause'], score.RESIDUAL_HISTORY)
        self.assertTrue(got['late_near_black'])
        self.assertFalse(got['true_black_tail'])

    def test_fade_uses_firmware_live_age_not_last_musical_sample(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (8, 9000, 80, 40, 0.05),
            (8, 400, 80, 136, 0.05),
            (120, 250, 0, 0, 0.05),
        ])
        hit_at = 2.5
        last_effect = 10
        for s in samples:
            s['musical'] = False
            s['visual_path'] = 'dwell'
            s['effect_frames'] = last_effect
            s['live_age_us'] = 0
            if abs(s['t'] - 2.90) < 0.03:
                s['musical'] = True
                s['visual_path'] = 'effect'
                s['effect_frames'] = 10
                s['live_age_us'] = 0
            if 2.95 < s['t'] < 3.20:
                last_effect = 25
                s['effect_frames'] = 25
                s['visual_path'] = 'dwell'
                s['live_age_us'] = int((s['t'] - 2.95) * 1e6)
            if s['t'] >= 3.20 and s['led_nz'] == 0:
                s['live_age_us'] = 3424801
        got = score.classify(samples, hit_at_s=hit_at)
        self.assertEqual(got['fade_source'], 'live_age_us')
        self.assertAlmostEqual(got['fade_s'], 3.425, places=3)

    def test_true_black_requires_zero_pixels(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (10, 8000, 80, 40, 0.05),
            (20, 400, 20, 10, 0.05),
            (120, 220, 0, 0, 0.05),
        ])
        got = score.classify(samples, hit_at_s=2.5)
        self.assertEqual(got['cause'], score.TRUE_BLACK)
        self.assertTrue(got['true_black_tail'])

    def test_elevated_hop_after_hit_is_fresh_sound(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (10, 8000, 80, 40, 0.05),
            (160, 6000, 50, 80, 0.05),
        ])
        got = score.classify(samples, hit_at_s=2.5)
        self.assertEqual(got['cause'], score.FRESH_SOUND)

    def test_loud_pre_hit_is_inconclusive_room(self):
        samples = series([
            (50, 6500, 15, 20, 0.05),
            (10, 7500, 80, 40, 0.05),
            (120, 4000, 1, 90, 0.05),
        ])
        got = score.classify(samples, hit_at_s=2.5)
        self.assertEqual(got['cause'], score.INCONCLUSIVE_ROOM)

    def test_emit_stall_fails_before_darkness_claim(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (10, 8000, 80, 40, 0.05),
            (120, 220, 0, 0, 0.05),
        ])
        for s in samples:
            if s['t'] >= 3.5:
                s['emitted'] = samples[60]['emitted']
        got = score.classify(samples, hit_at_s=2.5)
        self.assertEqual(got['cause'], score.FAIL_EMIT)

    def test_silence_window_excludes_hit_flash(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (8, 9000, 80, 40, 0.05),
            (8, 400, 80, 136, 0.05),
            (120, 250, 1, 14, 0.05),
        ])
        got = score.classify(samples, hit_at_s=2.5)
        self.assertEqual(got['cause'], score.RESIDUAL_HISTORY)
        self.assertLessEqual(got['silence_led_nz']['max'], 14)

    def test_first_hold_accepts_period_sampled_two_seconds(self):
        samples = series([(20, 300, 0, 0, 0.12)])
        hold = score.first_hold(
            samples, lambda s: s['last_hop_peak'] <= 1500, 2.0)
        self.assertIsNotNone(hold)
        self.assertGreaterEqual(hold[1] - hold[0], 2.0)

    def test_missed_hit_is_inconclusive(self):
        samples = series([
            (50, 200, 0, 0, 0.05),
            (10, 250, 0, 0, 0.05),
            (120, 210, 0, 0, 0.05),
        ])
        got = score.classify(samples, hit_at_s=2.5)
        self.assertEqual(got['cause'], score.INCONCLUSIVE_HIT)


if __name__ == '__main__':
    unittest.main()
