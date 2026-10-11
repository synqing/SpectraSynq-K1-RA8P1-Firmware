#!/usr/bin/env python3
"""Run with Python: pure host checks, no devices, services, media or subprocesses."""
import ast
import datetime
from pathlib import Path
import tempfile
import sys
from types import SimpleNamespace
import unittest

from capture_protocol import (anchored_deadline, capture_bounds, owned_command,
                              require_active, require_fresh_output, require_identity,
                              require_recording_complete, warm_child_outer)

CONSUMERS = None
if '--consumers' in sys.argv:
    index = sys.argv.index('--consumers')
    CONSUMERS = Path(sys.argv[index+1]).resolve()
    del sys.argv[index:index+2]


class CaptureProtocolTests(unittest.TestCase):
    def phase(self, record=72, playback=64, wall=90):
        return dict(record_seconds=record, playback_seconds=playback, wall_seconds=wall)

    def test_admitted_phases_and_monotonic_cutoff(self):
        for record, playback, wall in [(96,96,120),(108,108,120),(108,96,120),
                                      (48,48,66),(72,64,90),(112,112,120)]:
            with self.subTest(record=record,playback=playback):
                self.assertEqual(capture_bounds(self.phase(record,playback,wall),record,playback,
                                               wall,1000,60,100,20),20+wall)

    def test_wrong_duration_and_too_little_setup_slack(self):
        with self.assertRaises(ValueError):
            capture_bounds(self.phase(112,104,120),72,64,90,1000,60,100,20)
        with self.assertRaises(ValueError):
            capture_bounds(self.phase(72,64,79),72,64,79,1000,60,100,20)

    def test_finite_and_remaining_bounds(self):
        for outer, reserve in [(249,60),(1000,59),(float('nan'),60),(float('inf'),60)]:
            with self.subTest(outer=outer,reserve=reserve), self.assertRaises(ValueError):
                capture_bounds(self.phase(),72,64,90,outer,reserve,100,20)
        self.assertEqual(capture_bounds(self.phase(),72,64,90,250,60,100,20),110)

    def test_anchor_cannot_be_renewed(self):
        self.assertEqual(anchored_deadline(100,600,700,700),700)
        for recorded, declared in [(701,701),(700,800),(float('nan'),700)]:
            with self.subTest(recorded=recorded,declared=declared), self.assertRaises(ValueError):
                anchored_deadline(100,600,recorded,declared)

    def test_child_is_bounded_by_both_original_windows(self):
        self.assertEqual(warm_child_outer(1000,400,60,300,80),460)
        self.assertEqual(warm_child_outer(1000,1100,60,800,80),1000)
        for now in [321,355]:
            with self.subTest(now=now), self.assertRaises(ValueError):
                warm_child_outer(1000,400,60,now,80)

    def test_no_control_stage_after_child_or_deadline(self):
        require_active(99,100,None)
        for now,rc in [(100,None),(101,None),(99,0),(99,1),(float('nan'),None)]:
            with self.subTest(now=now,rc=rc), self.assertRaises(ValueError):
                require_active(now,100,rc)

    def test_failed_evidence_and_dangling_link_are_single_use(self):
        with tempfile.TemporaryDirectory() as root:
            root=Path(root); fresh=root/'fresh'; require_fresh_output(fresh)
            for existing in [root/'failed',root/'empty-directory',root/'dangling']:
                if existing.name=='failed': existing.write_text('FAILED_RETAINED')
                elif existing.name=='empty-directory': existing.mkdir()
                else: existing.symlink_to(root/'missing')
                with self.subTest(path=existing.name), self.assertRaises(ValueError):
                    require_fresh_output(existing)
            self.assertEqual((root/'failed').read_text(),'FAILED_RETAINED')

    def test_identity_is_fresh_owner_admission_not_source_equivalence(self):
        candidate={'uid':'target','build_id':'new-image','schema_sha256':'schema'}
        hello={'ok':True,'admitted':True,'bound':{'uid':'target','build':'new-image','schema_sha256':'schema'}}
        require_identity(candidate,hello)
        for field in ['uid','build','schema_sha256']:
            changed={**hello,'bound':{**hello['bound'],field:'stale'}}
            with self.subTest(field=field), self.assertRaises(ValueError):require_identity(candidate,changed)
        with self.assertRaises(ValueError):require_identity(candidate,{**hello,'admitted':False})

    def events(self,elapsed=72,error=''):
        start=datetime.datetime(2026,10,11,tzinfo=datetime.timezone.utc)
        return [{'event':'recording_started','utc':start.isoformat()},
                {'event':'recording_finished','utc':(start+datetime.timedelta(seconds=elapsed)).isoformat(),'error':error}]

    def test_delegate_start_excludes_preparation(self):
        require_recording_complete(self.events(72.01),72,0)
        for elapsed in [10.581,71.9]:
            with self.subTest(elapsed=elapsed), self.assertRaises(ValueError):
                require_recording_complete(self.events(elapsed),72,0)

    def test_partial_container_cancel_and_nonzero_child_cannot_pass(self):
        for events,rc,cancel in [(self.events(),1,False),(self.events(),None,False),(self.events(),0,True),
                                 (self.events(error='AVFoundation -11808'),0,False),
                                 (self.events()[:1],0,False),
                                 (self.events()+self.events(),0,False)]:
            with self.subTest(rc=rc,cancel=cancel), self.assertRaises(ValueError):
                require_recording_complete(events,72,rc,cancel)

    def test_exact_owned_argv_excludes_scanner_mentions(self):
        examples={'mixer':'python3 -m smc_titan_control live --port "SMC-Mixer Bluetooth"',
                  'broker':'/bin/python /tmp/titan_broker.py daemon --socket /tmp/owner',
                  'camera':'/tmp/product_camera capture id 48 /tmp/original.mov authorised',
                  'player':'/usr/bin/afplay /tmp/music.wav'}
        for kind,command in examples.items():
            with self.subTest(kind=kind):
                self.assertTrue(owned_command(command,kind))
                self.assertFalse(owned_command('python -c '+repr(command),kind))
                self.assertFalse(owned_command('/bin/echo '+command,kind))
        self.assertFalse(owned_command('/usr/bin/afplay','player'))
        self.assertFalse(owned_command('python titan_broker.py unrelated','broker'))

    def test_small_module_import_has_no_hardware_dependencies(self):
        tree=ast.parse(Path(__file__).with_name('capture_protocol.py').read_text())
        imports={n.names[0].name for n in tree.body if isinstance(n,ast.Import)}
        self.assertEqual(imports,{'datetime','math','os','shlex'})

    @unittest.skipUnless(CONSUMERS, 'Pass --consumers <existing V> for scratch caller inspection')
    def test_actual_consumer_calls_and_warm_argv(self):
        expected={'record_phase.py':{'capture_bounds','require_identity','require_fresh_output','require_recording_complete'},
                  'execute_direct_phase.py':{'require_active','require_fresh_output'},
                  'warm_start_phase.py':{'anchored_deadline','require_identity','require_fresh_output','owned_command','warm_child_outer'}}
        trees={}
        for filename,required in expected.items():
            source=(CONSUMERS/filename).read_text()
            tree=ast.parse(source); compile(tree,filename,'exec'); trees[filename]=tree
            calls={n.func.id for n in ast.walk(tree) if isinstance(n,ast.Call) and isinstance(n.func,ast.Name)}
            self.assertLessEqual(required,calls,filename)
        call=next(n for n in ast.walk(trees['record_phase.py']) if isinstance(n,ast.Call)
                  and isinstance(n.func,ast.Name) and n.func.id=='capture_bounds')
        env={'capture_bounds':capture_bounds,'phase':self.phase(),
             'a':SimpleNamespace(record_seconds=72,playback_seconds=64,wall_seconds=90,
                                 outer_deadline=1000,recovery_reserve=60),
             'phase_entry_epoch':100,'phase_entry_monotonic':20}
        self.assertEqual(eval(compile(ast.Expression(call),'actual-recorder-args','eval'),env),110)
        env['phase']=self.phase(112,104,120)
        with self.assertRaises(ValueError):
            eval(compile(ast.Expression(call),'actual-recorder-args','eval'),env)
        tree=trees['warm_start_phase.py']
        assignments=[n for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='child_outer' for t in n.targets)]
        self.assertEqual(len(assignments),1)
        final=tree.body[-1].value
        # Evaluate only the actual argv expression and its pure bound assignment.
        # No launchctl/socket/Owner/subprocess statement is executed.
        env={'warm_child_outer':warm_child_outer,'outer':1000,'deadline':400,
             'time':SimpleNamespace(time=lambda:300),'sys':SimpleNamespace(executable='/python'),
             'v':CONSUMERS,'flag':'enabled','a':SimpleNamespace(plan=Path('/plan'),version='fresh')}
        exec(compile(ast.Module(body=assignments,type_ignores=[]),'bound-only','exec'),env)
        argv=eval(compile(ast.Expression(final.args[0]),'argv-only','eval'),env)
        self.assertEqual(argv[2:4],['music-ml-enabled','fresh'])
        values=dict(zip(argv[4::2],argv[5::2]))
        self.assertEqual(values,{'--plan':'/plan','--record-seconds':'48','--playback-seconds':'48',
                                 '--wall-seconds':'66','--outer-deadline':'460','--recovery-reserve':'60'})
        env['time']=SimpleNamespace(time=lambda:355)
        with self.assertRaises(ValueError):
            exec(compile(ast.Module(body=assignments,type_ignores=[]),'bound-only','exec'),env)


if __name__=='__main__':
    unittest.main(verbosity=2)
