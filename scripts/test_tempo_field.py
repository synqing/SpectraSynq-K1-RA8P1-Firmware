#!/usr/bin/env python3
"""Independent host mathematical/lifecycle acceptance. No target access."""
import ctypes as C
import importlib.util
import math
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from fractions import Fraction

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('oracle', ROOT/'tools/audio/phase_oracle.py')
oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle)
U64=C.c_ulonglong

class TempoOracle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(prefix='tempo-independent-')
        library=Path(cls.tmp.name)/'tempo.so'
        implementation=ROOT/'src/k1/core/audio/tempo_phase_tracks.cpp'
        mutation=os.environ.get('AP_TEMPO_PHASE_MUTATION')
        if mutation:
            text=implementation.read_text()
            old,new={
                'phase_sign':('std::atan2(s,c)','std::atan2(-s,c)'),
                'time_endpoint':('samples.size()-1-i','samples.size()-i'),
                'freshness':('now.frame_index-t.last_observed_frame > kTempoPhaseCoastFrames','false'),
            }[mutation]
            if old not in text:raise RuntimeError('Mutation no longer applicable: '+mutation)
            implementation=Path(cls.tmp.name)/'mutated_phase.cpp'
            implementation.write_text(text.replace(old,new))
        subprocess.run(['c++','-std=c++17','-O2','-shared','-fPIC','-I'+str(ROOT/'src/k1'),
                        str(ROOT/'tests/audio/tempo_oracle_bridge.cpp'),
                        str(implementation),
                        str(ROOT/'src/k1/core/audio/tempo_field_publisher.cpp'),
                        str(ROOT/'src/k1/core/audio/tempo_tracker.cpp'),
                        str(ROOT/'src/k1/core/audio/tempo_acf.cpp'),
                        str(ROOT/'src/k1/core/audio/musical_time.cpp'),'-o',str(library)],check=True)
        cls.lib=C.CDLL(str(library))
        cls.lib.phase_observe.argtypes=[C.POINTER(C.c_float),C.c_float,U64,C.POINTER(C.c_double)]
        cls.lib.phase_derive.argtypes=[U64,C.c_double,C.c_uint,C.c_uint,U64,U64,C.POINTER(U64),C.POINTER(C.c_double)]
        cls.lib.phase_project.argtypes=[U64,C.c_double,U64,U64,U64,U64,U64,U64,C.c_int,C.POINTER(U64),C.POINTER(C.c_double)]

    @classmethod
    def tearDownClass(cls): cls.tmp.cleanup()

    def observe(self,values,bpm):
        values=(C.c_float*512)(*values); out=C.c_double()
        ok=self.lib.phase_observe(values,bpm,1080,C.byref(out))
        return ok,out.value

    def test_all_impulse_positions_AP_AC04(self):
        for bpm in (60,90,120,155,127.43,89.5):
            # Input is float in the public API. Test arithmetic at that exact
            # represented frequency, without hiding input quantisation in epsilon.
            bpm=C.c_float(bpm).value
            for amplitude in (2**-10,1,2**10):
                for pos in range(512):
                    values=[0.0]*512;values[pos]=amplitude
                    expected=oracle.correlate(values,bpm)
                    ok,actual=self.observe(values,bpm)
                    self.assertTrue(ok,(bpm,amplitude,pos))
                    error=oracle.circular_error(actual,expected['phase'])
                    self.assertLessEqual(error,expected['tolerance_cycles'],(bpm,amplitude,pos,error,expected))
                    exact=float(oracle.impulse_phase(pos,bpm))
                    self.assertLessEqual(oracle.circular_error(actual,exact),expected['tolerance_cycles'])

    def test_fractional_pulse_trains_AP_AC04(self):
        for bpm in (60,90,120,155,127.43,89.5):
            for offset in (0,1,2,17):
                values=[0.0]*512
                period=Fraction(60*48000,1080)/Fraction(str(bpm))
                n=0
                while True:
                    position=offset+int(n*period+Fraction(1,2))
                    if position>=512:break
                    values[position]=1.0;n+=1
                frequency=C.c_float(bpm).value
                expected=oracle.correlate(values,frequency)
                ok,actual=self.observe(values,frequency)
                self.assertTrue(ok)
                self.assertLessEqual(oracle.circular_error(actual,expected['phase']),expected['tolerance_cycles'])

    def test_invalid_observation_AP_AC09(self):
        for bad in (0.0,-1.0,math.nan,math.inf):
            values=[0.0]*512;values[73]=bad
            self.assertFalse(self.observe(values,120)[0],bad)
        values=[0.0]*512;values[73]=1
        for bpm in (0,-1,math.nan,math.inf): self.assertFalse(self.observe(values,bpm)[0])

    def test_exact_extended_ratios_AP_AC07(self):
        for beat in (0,1,2,3,2**32+1,2**53+1,2**63-1):
            for phase in (0,.25,.5,.75,1-2**-20):
                for p,q in ((1,2),(2,1),(3,2)):
                    b=U64();v=C.c_double()
                    ok=self.lib.phase_derive(beat,phase,p,q,7,7,C.byref(b),C.byref(v))
                    exact=(Fraction(beat)+Fraction(phase))*p/q
                    self.assertTrue(ok,(beat,phase,p,q))
                    self.assertEqual(b.value,exact.numerator//exact.denominator)
                    self.assertEqual(v.value,float(exact%1),(beat,phase,p,q))

    def test_invalid_derived_AP_AC07(self):
        for beat,phase,p,q,e in ((0,0,1,0,7),(0,0,0,1,7),(0,0,1,2,8),
                                 (2**64-1,0,2,1,7),(0,math.nan,1,2,7),
                                 (0,-.1,1,2,7),(0,1,1,2,7)):
            b=U64();v=C.c_double()
            self.assertFalse(self.lib.phase_derive(beat,phase,p,q,7,e,C.byref(b),C.byref(v)))

    def test_projection_cadence_and_boundary_AP_AC04(self):
        period=24000<<32
        for age in (0,1,23999,24000,47999,48000):
            b=U64();v=C.c_double()
            ok=self.lib.phase_project(3,.25,period,100000,100000,100000+age,7,7,1,C.byref(b),C.byref(v))
            self.assertTrue(ok,age)
            exact=Fraction(13,4)+Fraction(age,24000)
            self.assertEqual(b.value,int(exact))
            self.assertLessEqual(abs(v.value-float(exact%1)),8*2**-53)
        for frame,epoch,valid in ((148001,7,1),(99999,7,1),(100000,8,1),(100000,7,0)):
            b=U64();v=C.c_double()
            self.assertFalse(self.lib.phase_project(3,.25,period,100000,100000,frame,7,epoch,valid,C.byref(b),C.byref(v)))

    def test_oracle_mutation_sensitivity(self):
        x=[0.0]*512;x[7]=1
        reference=oracle.correlate(x,127.43)
        for mutated in (-reference['phase']%1,(reference['phase']+127.43/60*.0225)%1):
            self.assertGreater(oracle.circular_error(mutated,reference['phase']),reference['tolerance_cycles'])

    def test_lifecycle_AP_AC06_08_09_10(self):
        names=['age_boundaries','rank_identity','correction_boundary','epoch_and_suspend',
               'zero_coast_stale','capacity_ties','correction_wrap_parity','invalid_evidence',
               'negative_correction_wrap','generation_overflow','malformed_capacity',
               'silence_retained_energetic_history']
        for scenario,name in enumerate(names):
            with self.subTest(name=name):self.assertEqual(self.lib.phase_lifecycle(scenario),0,name)

    def test_publication_AP_AC02_03_08_09(self):
        names=['cached_delayed_immutable','sliced_epoch_rejection','disable_phase_toggle',
               'activity_overflow','nonmonotonic_availability']
        for scenario,name in enumerate(names):
            with self.subTest(name=name):self.assertEqual(self.lib.publication_case(scenario),0,name)

    def test_metadata_AP_AC03_08(self):
        names=['delayed_ring_snapshot','epoch_during_job','configuration_during_job',
               'generation_overflow','actual_unsliced_cadence']
        for scenario,name in enumerate(names):
            with self.subTest(name=name):self.assertEqual(self.lib.evidence_case(scenario),0,name)

    def test_discontinuity_full_history_AP_AC03_08(self):
        for config_change in (0,1):
            with self.subTest(configuration_change=config_change):
                self.assertEqual(self.lib.discontinuity_warmup_case(config_change),0)

    def test_consumer_AP_AC12(self):
        binary=Path(self.tmp.name)/'consumer'
        subprocess.run(['c++','-std=c++17','-O2','-I'+str(ROOT/'src/k1'),
                        str(ROOT/'tests/audio/tempo_reference_scene_test.cpp'),
                        str(ROOT/'src/k1/core/visual/tempo_reference_scene.cpp'),
                        str(ROOT/'src/k1/core/audio/tempo_phase_tracks.cpp'),
                        str(ROOT/'src/k1/core/audio/musical_time.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

if __name__=='__main__':unittest.main(verbosity=2)
