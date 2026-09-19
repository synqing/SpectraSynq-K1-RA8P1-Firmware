import json
import shutil
import sys
import tempfile
from pathlib import Path
import unittest

ROOT=Path(__file__).parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from run_scalar_schedule import (
    decode_raw_trace_chunk,
    summarise_raw_trace,
    validate_stage_profile,
)
from stage_profile import instrument_stage_sources


class StageProfileTests(unittest.TestCase):
    def test_disposable_sources_are_instrumented_without_touching_imports(self):
        names=[
            'core/audio/audio_pipeline.cpp',
            'core/audio/tempo_tracker.cpp',
            'core/audio/tempo_acf.cpp',
            'core/audio/clock_affine.cpp',
        ]
        originals={name:(ROOT/'src/k1'/name).read_bytes() for name in names}
        with tempfile.TemporaryDirectory() as temporary:
            staged=Path(temporary)/'k1'
            for name in names:
                target=staged/name
                target.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(ROOT/'src/k1'/name,target)
            transformed=instrument_stage_sources(staged)
            self.assertEqual(set(transformed),set(names))
            for name in names:
                text=(staged/name).read_text()
                self.assertIn('#include "stage_probe.h"',text)
                self.assertTrue(
                    'K1_STAGE_BEGIN(' in text or 'K1_STAGE_SCOPE(' in text
                )
                self.assertEqual((ROOT/'src/k1'/name).read_bytes(),originals[name])

    def test_profile_schema_and_counts_fail_closed(self):
        full={
            'ap_total','gdft_raw','gdft_postprocess','features','chord_detect',
            'onset_beat','musical_saliency','tempo_total','musical_time','telemetry',
        }
        updates={
            'tempo_history','tempo_acf_total','acf_prepare','acf_correlate',
            'acf_comb','acf_normalise','tempo_bank','tempo_flywheel','tempo_output',
        }
        measurement=lambda count:{
            'count':count,'mean_cycles':1.0,'p50_bin_lower_cycles':0,
            'p95_bin_lower_cycles':0,'p99_bin_lower_cycles':0,'max_cycles':2,
        }
        stages={name:measurement(6000) for name in full}
        stages.update({name:measurement(1999) for name in updates})
        stages['vp_render']=measurement(5400)
        stages['clock_affine']=measurement(0)
        status={'stage_profile':{
            'bin_width_cycles':16384,
            'raw_trace':{'version':2,'records':6000,'stride':112},
            'stages':stages,
        }}
        validate_stage_profile(status,6000,5400,6000)
        divergent=json.loads(json.dumps(status))
        del divergent['stage_profile']['stages']['acf_comb']
        with self.assertRaisesRegex(RuntimeError,'stage set'):
            validate_stage_profile(divergent,6000,5400)
        empty=json.loads(json.dumps(status))
        for name in updates: empty['stage_profile']['stages'][name]['count']=0
        with self.assertRaisesRegex(RuntimeError,'counts'):
            validate_stage_profile(empty,6000,5400)

    def test_raw_trace_decode_and_summary(self):
        stages=['tempo_total','tempo_acf_total']
        stride=(7+len(stages))*4
        header=b'K1T1'+(2).to_bytes(4,'little')+(0).to_bytes(4,'little')
        header+=(2).to_bytes(4,'little')+stride.to_bytes(4,'little')
        header+=len(stages).to_bytes(4,'little')
        words=[0,9000,0,0,0,0,0,1000,0,1,12000,500,1|4,0,40,800,6000,5000]
        body=header+b''.join(word.to_bytes(4,'little') for word in words)
        records=decode_raw_trace_chunk(body,0,stages)
        summary=summarise_raw_trace(records,stages,1_000_000)
        self.assertEqual(summary['cross_tab'],{
            'tempo_missed':1,'tempo_met':0,'ordinary_missed':0,'ordinary_met':1,
        })
        self.assertEqual(summary['total']['tempo']['max_us'],12000.0)
        self.assertEqual(
            summary['derived_tempo_tracker_exclusive']['tempo']['max_cycles'],
            1000,
        )
        self.assertEqual(summary['software_double']['tempo']['calls']['max_cycles'], 40)
        self.assertEqual(summary['software_double']['tempo']['cycles']['max_cycles'], 800)
        with self.assertRaisesRegex(RuntimeError,'shape'):
            decode_raw_trace_chunk(body[:-1],0,stages)


if __name__=='__main__': unittest.main()
