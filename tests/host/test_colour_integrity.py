"""HOST negative controls; no serial port, GPIO or room player is opened."""
import copy
import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch, Mock
import zlib

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import run_colour_integrity as colour


def gpt(count):
    return dict(attempts=count + 1, dma_irqs=count, stop_irqs=count, frames=count,
                first_fault=0, errors=0, fault_witness=dict(valid=0), bits=3072,
                dmctl=1, maximum_prepare_cycles=100, clock_hz=240000000)


def platform(count):
    pdm = dict(initialised=True, running=True, sample_rate_hz=40000,
               slot_elements=296, profile='ap_40k_asrc24', last_fsp_error=0,
               **{k: 0 for k in colour.PDM_BAD}, ap_hops=count, paired_slots=count,
               last_capture_end_us=count * 7500)
    pdm['lanes'] = [dict(dma_channel=i, error_flags=0, data_callbacks=count,
                         processed_slots=count, sat_neg=count, sat_pos=count,
                         **{k: 0 for k in colour.LANE_BAD}) for i in range(2)]
    return dict(pdm_target=pdm)


class ColourIntegrity(unittest.TestCase):
    def test_build_without_declared_lane_map_is_rejected(self):
        with tempfile.TemporaryDirectory(prefix='k1-colour-build-') as temp:
            path = Path(temp)
            receipt = dict(pass_=True, pdm_target=True, palette_gpt_dma=True,
                           source_pin=colour.PIN, dmac_priority='fixed',
                           dmac_control_initialisation=dict(live_policy_change='REFUSED'))
            receipt['pass'] = receipt.pop('pass_')
            (path / 'receipt.json').write_text(json.dumps(receipt))
            with self.assertRaisesRegex(ValueError, 'source-bound'):
                colour.accepted_build(path)

    def test_valid_progress_and_both_raw_rails_retained(self):
        result = colour.score_phase(gpt(10), gpt(20), platform(10), platform(20), 1)
        self.assertEqual(result['gpt']['frames'], 10)
        self.assertEqual(result['lanes'][1]['sat_neg_delta'], 10)
        self.assertEqual(result['lanes'][1]['sat_pos_delta'], 10)
        self.assertEqual(result['waveform'], 'NOT_CAPTURED')

    def test_led_first_lane_identity_is_explicit(self):
        first, second = platform(10), platform(20)
        for record in (first, second):
            for index, lane in enumerate(record['pdm_target']['lanes']):
                lane['dma_channel'] = index + 1
        g0, g1 = dict(gpt(10), dmctl=0), dict(gpt(20), dmctl=0)
        colour.score_phase(g0, g1, first, second, 0, 'led-first')
        with self.assertRaisesRegex(ValueError, 'DMA channel identity'):
            colour.score_phase(g0, g1, platform(10), platform(20), 0,
                               'led-first')

    def test_missing_or_changed_mandatory_pdm_counters_never_pass(self):
        keys = colour.PDM_BAD + ['ap_hops', 'paired_slots', 'last_capture_end_us']
        for key in keys:
            with self.subTest(key=key):
                second = platform(20)
                del second['pdm_target'][key]
                with self.assertRaises((KeyError, ValueError)):
                    colour.score_phase(gpt(10), gpt(20), platform(10), second, 1)
        for lane in range(2):
            for key in colour.LANE_BAD + ['error_flags', 'data_callbacks', 'processed_slots', 'sat_neg', 'sat_pos']:
                with self.subTest(lane=lane, key=key):
                    second = platform(20)
                    del second['pdm_target']['lanes'][lane][key]
                    with self.assertRaises((KeyError, ValueError)):
                        colour.score_phase(gpt(10), gpt(20), platform(10), second, 1)
        for key in colour.PDM_BAD:
            second = platform(20)
            second['pdm_target'][key] = 1
            with self.assertRaises(ValueError):
                colour.score_phase(gpt(10), gpt(20), platform(10), second, 1)
        for lane in range(2):
            for key in colour.LANE_BAD:
                second = platform(20)
                second['pdm_target']['lanes'][lane][key] = 1
                with self.assertRaises(ValueError):
                    colour.score_phase(gpt(10), gpt(20), platform(10), second, 1)

    def test_fault_false_completion_policy_and_stalls_rejected(self):
        for changes in (dict(first_fault=6), dict(errors=1),
                        dict(fault_witness=dict(valid=1)), dict(frames=21),
                        dict(dmctl=0), dict(bits=24)):
            changed = dict(gpt(20), **changes)
            with self.assertRaises(ValueError):
                colour.score_phase(gpt(10), changed, platform(10), platform(20), 1)
        with self.assertRaises(ValueError):
            colour.score_phase(gpt(10), gpt(10), platform(10), platform(20), 1)
        with self.assertRaises(ValueError):
            colour.score_phase(gpt(10), gpt(20), platform(10), platform(10), 1)
        second = platform(20)
        second['pdm_target']['lanes'][1]['sat_neg'] = 0
        with self.assertRaises(ValueError):
            colour.score_phase(gpt(10), gpt(20), platform(10), second, 1)

    def test_settings_are_explicit_and_round_trip_layout(self):
        settings = dict(active=True, automatic_cycle=False, emit_enabled=True,
                        showcase=False, direction='centre_out', palette_a=0,
                        palette_b=1, mode_a=32, mode_b=32, brightness=255,
                        output_channel=0, transition_ms=0, travel_ms=4000)
        self.assertEqual(struct.unpack('<10I', colour.settings_payload(settings)),
                         (3, 0, 1, 32, 32, 5, 255, 0, 0, 4000))
        colour.check_settings(copy.deepcopy(settings), settings)
        for key, value in [('brightness', 256), ('direction', 'linear'), ('active', 1)]:
            with self.assertRaises(ValueError):
                colour.settings_payload(dict(settings, **{key: value}))
        with self.assertRaises(ValueError):
            colour.check_settings(dict(settings, mode_a=64), settings)

    def test_busy_cdc_is_not_opened_and_wrong_identity_never_configured(self):
        device = SimpleNamespace(device='/dev/cu.test', vid=0x045b, pid=0x5310)
        fake_ports = SimpleNamespace(comports=lambda: [device])
        serial_module = SimpleNamespace(Serial=Mock())
        modules = {'serial': serial_module, 'serial.tools': SimpleNamespace(list_ports=fake_ports)}
        with tempfile.TemporaryDirectory(prefix='k1-colour-owner-') as temp:
            with patch.dict(sys.modules, modules), patch.object(colour, 'accepted_build', return_value=(
                    dict(build_id='expected', dmac_priority='fixed'), 'hex')), \
                    patch.object(colour.subprocess, 'run', return_value=SimpleNamespace(returncode=0, stdout='123\n')) as owners, \
                    patch.object(sys, 'argv', ['runner', '--build', temp, '--output', temp + '/busy']):
                with self.assertRaisesRegex(ValueError, 'CDC owned'):
                    colour.main()
                serial_module.Serial.assert_not_called()
                self.assertEqual(owners.call_args.args[0][-2:], ['/dev/cu.test', '/dev/tty.test'])
                self.assertFalse(json.loads(Path(temp + '/busy/receipt.json').read_text())['pass'])
            class Port:
                def __init__(self):
                    self.responses = bytearray()
                    self.ops = []
                    self.closed = False
                def write(self, message):
                    op, rid = struct.unpack_from('<2I', message, 4)
                    self.ops.append(op)
                    body = json.dumps(dict(uid='WRONG', build='expected', source=colour.PIN, protocol=1)).encode()
                    head = struct.pack('<4s6I', b'K1R1', 0, rid, 0, len(body), 0, zlib.crc32(body))
                    self.responses.extend(head + struct.pack('<I', zlib.crc32(head)) + body)
                    return len(message)
                def flush(self):
                    pass
                def read(self, size):
                    out = bytes(self.responses[:size])
                    del self.responses[:size]
                    return out
                def close(self):
                    self.closed = True
            port = Port()
            serial_module.Serial.return_value = port
            with patch.dict(sys.modules, modules), patch.object(colour, 'accepted_build', return_value=(
                    dict(build_id='expected', dmac_priority='fixed'), 'hex')), \
                    patch.object(colour.subprocess, 'run', return_value=SimpleNamespace(returncode=1, stdout='')), \
                    patch.object(sys, 'argv', ['runner', '--build', temp, '--output', temp + '/wrong']):
                with self.assertRaisesRegex(ValueError, 'UID/build'):
                    colour.main()
            self.assertEqual(port.ops, [1])
            self.assertTrue(port.closed)

    def test_capture_error_and_restore_error_still_release_cdc(self):
        settings = dict(active=True, automatic_cycle=False, emit_enabled=True,
                        showcase=False, direction='centre_out', palette_a=0,
                        palette_b=1, mode_a=32, mode_b=32, brightness=255,
                        output_channel=0, transition_ms=0, travel_ms=4000,
                        bench_pixels=128, wire_profile=1)
        class Port:
            def __init__(self, fail_restore):
                self.settings = settings.copy()
                self.fail_restore = fail_restore
                self.configures = 0
                self.responses = bytearray()
                self.closed = False
            def write(self, message):
                op, rid = struct.unpack_from('<2I', message, 4)
                if op == 1:
                    value = dict(uid=colour.UID, build='expected', source=colour.PIN, protocol=1)
                elif op == 17:
                    value = self.settings
                elif op == 16:
                    self.configures += 1
                    if self.configures == 2 and self.fail_restore:
                        raise ValueError('injected restore failure')
                    vals = struct.unpack_from('<10I', message, 32)
                    self.settings.update(zip(['palette_a', 'palette_b', 'mode_a', 'mode_b'], vals[1:5]))
                    self.settings.update(zip(['brightness', 'output_channel', 'transition_ms', 'travel_ms'], vals[6:]))
                    value = self.settings
                elif op == 22:
                    value = {'invalid': 'injected capture failure'}
                else:
                    raise AssertionError('unexpected opcode ' + str(op))
                body = json.dumps(value).encode()
                head = struct.pack('<4s6I', b'K1R1', 0, rid, 0, len(body), 0, zlib.crc32(body))
                self.responses.extend(head + struct.pack('<I', zlib.crc32(head)) + body)
                return len(message)
            def flush(self):
                pass
            def read(self, size):
                out = bytes(self.responses[:size])
                del self.responses[:size]
                return out
            def close(self):
                self.closed = True
        device = SimpleNamespace(device='/dev/cu.test', vid=0x045b, pid=0x5310)
        for fail_restore in (False, True):
            with self.subTest(fail_restore=fail_restore), tempfile.TemporaryDirectory(prefix='k1-colour-restore-') as temp:
                port = Port(fail_restore)
                modules = {'serial': SimpleNamespace(Serial=lambda *a, **kw: port),
                           'serial.tools': SimpleNamespace(list_ports=SimpleNamespace(comports=lambda: [device]))}
                with patch.dict(sys.modules, modules), patch.object(colour, 'accepted_build', return_value=(
                        dict(build_id='expected', dmac_priority='fixed'), 'hex')), \
                        patch.object(colour.subprocess, 'run', return_value=SimpleNamespace(returncode=1, stdout='')), \
                        patch.object(sys, 'argv', ['runner', '--build', temp, '--output', temp + '/run']):
                    with self.assertRaisesRegex(ValueError, 'layout mismatch'):
                        colour.main()
                receipt = json.loads(Path(temp + '/run/receipt.json').read_text())
                self.assertFalse(receipt['pass'])
                self.assertTrue(port.closed and receipt['cdc_released'])
                self.assertEqual(port.configures, 2)
                if fail_restore:
                    self.assertIn('injected restore failure', receipt['restore_error'])
                else:
                    self.assertTrue(receipt['restore_verified'])


if __name__ == '__main__':
    unittest.main()
