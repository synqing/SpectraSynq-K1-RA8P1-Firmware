#!/usr/bin/env python3
import copy, importlib.util
from pathlib import Path
spec = importlib.util.spec_from_file_location(
    'runner', Path(__file__).with_name('run_pdm_ap_gpt.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

def snap(hops, slots=10, end_us=1000, rearm=0):
    lane = {
        'overflow_events': 0, 'drop_events': 0,
        'sat_neg': 3, 'sat_pos': 0, 'packing_mismatch': 1, 'first_sat_raw': 0x8000,
        'recovery_count': 0,
    }
    return {
        'pdm_target': {
            'sample_rate_hz': 40000,
            'slot_elements': 296,
            'profile': 'ap_40k_asrc24',
            'rearm_denied': rearm,
            'ap_hops': hops,
            'paired_slots': slots,
            'last_capture_end_us': end_us,
            'recovery_count': 0,
            'asrc_starved': 0,
            'acoustic_identity': 'unproven',
            'lanes': [lane, dict(lane)],
        }
    }

delta = m.score(snap(2, end_us=1000), snap(12, 40, end_us=9000))
assert delta['ap_hops'] == 10
bad = copy.deepcopy(snap(12))
bad['pdm_target']['sample_rate_hz'] = 16000
try:
    m.score(snap(2), bad)
except ValueError:
    pass
else:
    raise SystemExit('16 kHz admission accepted')
print('K1_PDM_AP_GPT_SCORER_HOST=PASS')
