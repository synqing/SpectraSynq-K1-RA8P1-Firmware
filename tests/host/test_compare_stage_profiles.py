import json
import sys
import tempfile
from pathlib import Path
import unittest

ROOT=Path(__file__).parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from compare_stage_profiles import compare


def run_receipt(profile: bool) -> dict:
    result={
        name:{'mean_us':value}
        for name,value in {'total':110.0 if profile else 100.0,
                           'tempo':220.0 if profile else 200.0,
                           'ordinary':55.0 if profile else 50.0,
                           'render':11.0 if profile else 10.0}.items()
    }
    result['finished']=True
    if profile:
        result['stage_profile']={
            'bin_width_cycles':16384,
            'stages':{
                'acf_correlate':{
                    'count':2,'mean_cycles':48000.0,
                    'p50_bin_lower_cycles':32768,
                    'p95_bin_lower_cycles':49152,
                    'p99_bin_lower_cycles':49152,
                    'max_cycles':52000,
                }
            },
        }
    return {
        'measurement_validated':True,'loops':1,'mode':'scalar',
        'mutation':False,'qualification':False,
        'identities':{'profile_sha256':'fixture'},
        'runtime':{'uid':'board','source':'pin','clock_hz':480000000,
                   'build':'profile' if profile else 'control'},
        'result':result,
    }


class CompareStageProfilesTests(unittest.TestCase):
    def test_converts_cycles_and_reports_paired_overhead(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            for name,profile in (('profile',True),('control',False)):
                directory=root/name; directory.mkdir()
                (directory/'receipt.json').write_text(json.dumps(run_receipt(profile)))
            result=compare(root/'profile',root/'control')
            self.assertEqual(result['overhead']['total']['overhead_percent'],10.0)
            self.assertEqual(result['stage_measurements']['acf_correlate']['mean_us'],100.0)

    def test_rejects_unvalidated_receipt(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            for name,profile in (('profile',True),('control',False)):
                directory=root/name; directory.mkdir()
                receipt=run_receipt(profile)
                if profile: receipt['measurement_validated']=False
                (directory/'receipt.json').write_text(json.dumps(receipt))
            with self.assertRaisesRegex(RuntimeError,'unvalidated'):
                compare(root/'profile',root/'control')


if __name__=='__main__': unittest.main()
