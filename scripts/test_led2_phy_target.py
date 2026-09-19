#!/usr/bin/env python3
"""Host-only LED2 scorer and bounded observation tests; no serial access."""
from __future__ import annotations

import unittest
import struct

from run_led2_phy_target import (
    EXPECTED_CHANNELS, EXPECTED_DWT_HZ, TRACE_RECORD, collect_samples, observe_readiness,
    parse_firmware_trace, score_samples,
)


def sample(g=0, y=0, *, ok=True, phy_id=0x001CC916, id1=0x001C,
           id2=0xC916, addr=1, err=0, io=0):
    return dict(ok=ok, id=phy_id, id1=id1, id2=id2, addr=addr,
                g=g, y=y, err=err, io=io, applied_valid=True,
                reg_readback=True, page_unknown=False, dwt_ok=True,
                dwt_hz=EXPECTED_DWT_HZ, pins=0xff, txc_requested=True,
                txc_state='unverified')


def pending():
    return sample(ok=False, phy_id=0, id1=0, id2=0, addr=0, err=1)


class Clock:
    def __init__(self):
        self.now = 0.0
        self.sleeps = []

    def monotonic(self):
        return self.now

    def sleep(self, seconds):
        assert 0 < seconds <= 0.2
        self.sleeps.append(seconds)
        self.now += seconds


class StatusReader:
    def __init__(self, entries, repeat_last=False):
        self.entries = list(entries)
        self.repeat_last = repeat_last
        self.calls = 0

    def __call__(self):
        index = self.calls
        self.calls += 1
        if self.repeat_last:
            index = min(index, len(self.entries) - 1)
        entry = self.entries[index]
        if isinstance(entry, Exception):
            raise entry
        return {'led2': dict(entry)}


class TargetTests(unittest.TestCase):
    def observe(self, reader, receipt, clock, seconds=5, sample_ms=75):
        return observe_readiness(reader, receipt, seconds, sample_ms,
                                 clock.monotonic, clock.sleep)

    def test_original_sequence_cases(self):
        valid = [sample(g, y) for g, y in EXPECTED_CHANNELS for _ in range(2)]
        self.assertEqual(score_samples(valid), EXPECTED_CHANNELS)
        broken = [
            [sample(g, y) for g, y in [(0, 0), (1, 0), (0, 0), (1, 1), (0, 0)]],
            [sample(g, y) for g, y in [(0, 0), (0, 1), (1, 0), (0, 0),
                                      (0, 1), (0, 0), (1, 1), (0, 0)]],
            [sample(g, y, phy_id=0xffffffff) for g, y in EXPECTED_CHANNELS],
            [sample(g, y, addr=2) for g, y in EXPECTED_CHANNELS],
            [sample(g, y, err=2) for g, y in EXPECTED_CHANNELS],
            [sample(g, y, ok=False) for g, y in EXPECTED_CHANNELS],
            [sample(g, y, id1=0) for g, y in EXPECTED_CHANNELS],
            [sample(g, y, id2=0xffff) for g, y in EXPECTED_CHANNELS],
            [sample(g, y, id2=0xc917) for g, y in EXPECTED_CHANNELS],
            [],
        ]
        for trace in broken:
            with self.subTest(trace=trace), self.assertRaises(RuntimeError):
                score_samples(trace)

    def test_delayed_ready_retains_all_observations(self):
        reader = StatusReader([pending(), pending(), pending(), sample()])
        receipt, clock = {}, Clock()
        result = self.observe(reader, receipt, clock)
        observation = receipt['readiness']
        self.assertEqual(reader.calls, 4)
        self.assertEqual(len(observation['history']), 4)
        self.assertEqual([x['elapsed_ms'] for x in observation['history']], [0, 75, 150, 225])
        self.assertTrue(all(x['observed_at'] for x in observation['history']))
        self.assertTrue(observation['ready'])
        self.assertEqual(observation['firmware_retry_count'], 'NOT_OBSERVED')
        self.assertFalse(receipt['before']['led2']['ok'])
        self.assertEqual(receipt['ready_status'], result)

    def test_never_ready_stops_at_deadline(self):
        reader = StatusReader([pending()], repeat_last=True)
        receipt, clock = {}, Clock()
        with self.assertRaisesRegex(RuntimeError, 'readiness timeout'):
            self.observe(reader, receipt, clock, seconds=1)
        self.assertAlmostEqual(clock.now, 1)
        self.assertEqual(reader.calls, 14)
        self.assertEqual(len(receipt['readiness']['history']), 14)
        self.assertFalse(receipt['readiness']['ready'])
        self.assertNotIn('ready_status', receipt)
        self.assertEqual(receipt['readiness']['elapsed_ms'], 1000)

    def test_read_exception_preserves_history(self):
        reader = StatusReader([pending(), RuntimeError('USB failed')])
        receipt, clock = {}, Clock()
        with self.assertRaisesRegex(RuntimeError, 'USB failed'):
            self.observe(reader, receipt, clock)
        self.assertEqual(len(receipt['readiness']['history']), 1)
        self.assertEqual(receipt['readiness']['error'], 'USB failed')
        self.assertNotIn('ready_status', receipt)

    def test_bad_identity_blocks_readiness(self):
        cases = [sample(phy_id=0x12345678), sample(addr=2), sample(id2=0xc917),
                 sample(ok=False, phy_id=0x12345678, err=1),
                 sample(phy_id=0, id1=0, id2=0)]
        for bad in cases:
            with self.subTest(bad=bad):
                reader = StatusReader([bad, sample()])
                receipt, clock = {}, Clock()
                with self.assertRaises(RuntimeError):
                    self.observe(reader, receipt, clock)
                self.assertEqual(reader.calls, 1)
                self.assertEqual(len(receipt['readiness']['history']), 1)
                self.assertFalse(receipt['readiness']['ready'])
                self.assertNotIn('ready_status', receipt)

    def test_mode_readiness_may_arrive_after_identity(self):
        reader = StatusReader([sample(ok=False, err=4), sample()])
        receipt, clock = {}, Clock()
        self.observe(reader, receipt, clock)
        self.assertEqual(reader.calls, 2)

    def test_gpio_and_restore_errors_are_fatal(self):
        for change in ({'io': 1}, {'err': 2}, {'err': 3}):
            with self.subTest(change=change):
                bad = pending()
                bad.update(change)
                reader = StatusReader([bad, sample()])
                receipt, clock = {}, Clock()
                with self.assertRaisesRegex(RuntimeError, 'readiness failed'):
                    self.observe(reader, receipt, clock)
                self.assertEqual(reader.calls, 1)

    def test_reply_after_deadline_is_recorded_but_not_ready(self):
        receipt, clock = {}, Clock()
        def slow_read():
            clock.now += 6
            return {'led2': sample()}
        with self.assertRaisesRegex(RuntimeError, 'readiness timeout'):
            self.observe(slow_read, receipt, clock)
        self.assertEqual(len(receipt['readiness']['history']), 1)
        self.assertEqual(receipt['readiness']['elapsed_ms'], 6000)
        self.assertFalse(receipt['readiness']['ready'])

    def test_invalid_observation_arguments_do_not_read(self):
        for seconds, sample_ms in [(0, 75), (-1, 75), (float('nan'), 75),
                                   (float('inf'), 75), (61, 75), (5, 0), (5, 201)]:
            with self.subTest(seconds=seconds, sample_ms=sample_ms):
                reader, receipt, clock = StatusReader([sample()]), {}, Clock()
                with self.assertRaises(ValueError):
                    self.observe(reader, receipt, clock, seconds, sample_ms)
                self.assertEqual(reader.calls, 0)

    def test_partial_sequence_survives_read_failure(self):
        reader = StatusReader([sample(), sample(1, 0), RuntimeError('USB interrupted')])
        receipt, clock = {}, Clock()
        with self.assertRaisesRegex(RuntimeError, 'USB interrupted'):
            collect_samples(reader, receipt, 5, 75, clock.monotonic, clock.sleep)
        self.assertEqual([(x['g'], x['y']) for x in receipt['samples']], [(0, 0), (1, 0)])
        self.assertEqual([x['elapsed_ms'] for x in receipt['samples']], [0, 75])

    def test_binary_trace_is_strict_and_attributed(self):
        records = [
            (1, 7, 205, 0x001c, 1, 2, 0, 0, 1),
            (2, 7, 207, 0x0000, 4, 3, 1, 0, 0),
        ]
        body = b'L2T1' + struct.pack('<I', len(records)) + b''.join(
            TRACE_RECORD.pack(*record) for record in records)
        parsed = parse_firmware_trace(body)
        self.assertEqual(parsed[0], {
            'sequence': 1, 'attempt': 7, 'reset_age_ms': 205,
            'address': 1, 'register': 2, 'ack': 0, 'io_error': 0,
            'value': 0x001c, 'value_valid': True,
        })
        self.assertFalse(parsed[1]['value_valid'])
        for broken in (body[:-1], b'BAD!' + body[4:], body[:4] + struct.pack('<I', 97),
                       b'L2T1' + struct.pack('<I', 2) +
                       TRACE_RECORD.pack(*records[1]) + TRACE_RECORD.pack(*records[0])):
            with self.subTest(broken=broken), self.assertRaises(RuntimeError):
                parse_firmware_trace(broken)

    def test_ready_requires_firmware_register_readback(self):
        for change in ({'applied_valid': False}, {'reg_readback': False},
                       {'page_unknown': True}):
            bad = sample()
            bad.update(change)
            with self.subTest(change=change), self.assertRaises(RuntimeError):
                score_samples([bad])

    def test_ready_requires_timing_pin_and_clock_route_diagnostics(self):
        for change in ({'dwt_ok': False}, {'dwt_hz': 999_999_999}, {'pins': 0xfe},
                       {'txc_requested': False}, {'txc_state': 'enabled'}):
            bad = sample()
            bad.update(change)
            with self.subTest(change=change), self.assertRaises(RuntimeError):
                score_samples([bad])


if __name__ == '__main__':
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(TargetTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        raise SystemExit(1)
    print('K1_LED2_PHY_TARGET_SCORER=PASS')
