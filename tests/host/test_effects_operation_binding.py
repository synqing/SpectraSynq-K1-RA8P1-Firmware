"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ORCH-0 scope only, no product admission."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import pytest

W=Path(__file__).resolve().parents[2]
E=W/'docs/effects-language-001/evidence'
BASE=json.loads((E/'operation-baseline-001.json').read_text())
SESSION='31b71435bd49aaa2805452dc'
TASKS={'ORCH-0','VP-1','VP-2','VP-3','MOD-1','MOD-SLICE','IVV-SLICE'}
def sha(body):return hashlib.sha256(body).hexdigest()
def command(*args):return subprocess.check_output(args,cwd=W,text=True,timeout=30)

def check_sources(record):
    assert command('git','rev-parse','HEAD').strip()==record['checkpoint']
    assert command('git','branch','--show-current').strip()==record['branch']
    for path,digest in record['protected_baseline'].items():
        assert sha((W/path).read_bytes())==digest,'protected source changed: '+path
    manifest=json.loads((W/'docs/effects-language-001/lane-inputs.json').read_text())
    assert len(manifest['records'])==record['retained_inputs_checked']
    for row in manifest['records']:
        assert sha((W/row['path']).read_bytes())==row['sha256'],'retained input changed'

def check_binding(report):
    assert report['schema']=='assignment-report.v1' and report['read_only']
    assert report['session_key']==SESSION,'wrong session'
    assert Path(report['project'])==W.parents[1]
    assert {t['id'] for t in report['tasks']}==TASKS,'wrong assigned tasks'
    assert report['gate']['registry_bound']
    row=next(t for t in report['tasks'] if t['id']=='ORCH-0')
    assert row['owner']=='ORCH' and row['required_cases']==['ORCH-0']

def test_retained_inputs_and_protected_base_ORCH0():
    check_sources(BASE)
    assert str(W)==BASE['worktree']
    for name in ['START-HERE.md','DUAL-EDGE-CORRECTION.md']:
        text=(W/'docs/effects-language-001'/name).read_text()
        assert text.startswith('THE TITAN MUST ALWAYS BE VISUALISING MUSIC.')

def test_full_assignments_and_explicit_owners_ORCH0():
    before=json.loads((E/'original-registry.json').read_text())
    after=json.loads((W/BASE['registry']).read_text())
    # Execution state/evidence may change; full immutable task assignments may not.
    mutable={'state','status','evidence_ref','evidence_sha256','blocker','unblocked_by'}
    for registry in [before,after]:
        for task in registry['tasks']:
            for key in mutable:task.pop(key,None)
            if task['id']=='ORCH-0':task.pop('verification_contract',None)
    assert before==after,'original assignment changed'
    assert sha((E/'original-registry.json').read_bytes())==BASE['registry_sha256']
    paths=[]
    for owner,owned in BASE['ownership'].items():
        assert owner=='primary_native_codex'
        for path in owned:
            assert (W/path).is_file() and path not in paths
            assert not path.startswith('platform/')
            paths.append(path)
    assert 'not launched' in BASE['reference_review']
    assert 'platform owner' in BASE['hardware']

def test_actual_native_binding_ORCH0():
    report=json.loads(command('/Users/spectrasynq/.local/bin/agentctl','integration','status','--session',SESSION))
    check_binding(report)
    print(json.dumps({'observed_session':report['session_key'],'observed_tasks':[t['id'] for t in report['tasks']],'read_only':report['read_only']}))

def test_wrong_source_and_session_rejected_ORCH0():
    wrong=copy.deepcopy(BASE)
    wrong['protected_baseline'][next(iter(wrong['protected_baseline']))]='0'*64
    with pytest.raises(AssertionError,match='protected source changed'):check_sources(wrong)
    report=json.loads((E/'actual-binding-001.json').read_text());report['session_key']='0'*24
    with pytest.raises(AssertionError,match='wrong session'):check_binding(report)
