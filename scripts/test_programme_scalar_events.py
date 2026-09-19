#!/usr/bin/env python3
"""Run the real programming wrapper with a fake ROM transport. Never opens USB."""
import contextlib,hashlib,importlib.util,io,json,sys,tempfile,types
from pathlib import Path
from unittest.mock import patch
calls=[]
boot=types.ModuleType('titan_ra8p1_boot');boot.__file__=__file__
boot.parse_ihex=lambda p:types.SimpleNamespace(sha256='candidate',start=0x02000000,end=0x020000ff)
boot._select_port=lambda p,t:('/dev/cu.fake',{'vid':0x045b,'pid':0x0261})
boot._open=lambda p:types.SimpleNamespace(close=lambda:calls.append('close'))
boot.RA8P1Boot=lambda p:p
boot._identify=lambda d,u:(types.SimpleNamespace(device_id='545433931bd25436593630352d068363'),[],None)
def programme(*a):calls.append('write')
boot._programme=programme
sys.modules['titan_ra8p1_boot']=boot
spec=importlib.util.spec_from_file_location('programmer',Path(__file__).with_name('programme_scalar.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
read=Path.read_bytes;sha=hashlib.sha256
restore_hash='a167833b7f35f2efa8ba296c772ab59c477bda2f2621faabe5510500aab5929a'
def fake_read(p):
    return b'fake-restore' if str(p).endswith('p3-2026-09-09/build-staged-v2/rtthread.hex') else read(p)
def fake_sha(data):
    return types.SimpleNamespace(hexdigest=lambda:restore_hash) if data==b'fake-restore' else sha(data)
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp);build=root/'build';build.mkdir()
    (build/'receipt.json').write_text(json.dumps({'pass':True,'build_id':'new','artifacts':{'rtthread.hex':'candidate'}}))
    for mode in ['ok','wrong-uid','verify-fail','retired']:
        calls.clear();out=root/mode
        boot._identify=lambda d,u:(types.SimpleNamespace(device_id='wrong' if mode=='wrong-uid' else m.UID),[],None)
        def prog(*a):
            calls.append('write')
            if mode=='verify-fail':raise RuntimeError('readback mismatch')
        boot._programme=prog
        retired='42bd1d6d8f2ece9882ec3cfbd64778535cdd1e33aa10b04c024549f3bc2ee75e'
        image_sha=retired if mode=='retired' else 'candidate'
        boot.parse_ihex=lambda p:types.SimpleNamespace(sha256=image_sha,start=0x02000000,end=0x020000ff)
        (build/'receipt.json').write_text(json.dumps({'pass':True,'build_id':'new','artifacts':{'rtthread.hex':image_sha}}))
        text=io.StringIO()
        with patch.object(sys,'argv',['programme_scalar.py','--build',str(build),'--output',str(out),'--execute']),patch.object(Path,'read_bytes',fake_read),patch.object(m.hashlib,'sha256',fake_sha),patch.object(m.subprocess,'run',return_value=types.SimpleNamespace(returncode=1,stdout='',stderr='')),contextlib.redirect_stdout(text):
            try:m.main()
            except RuntimeError:
                assert mode!='ok'
        events=[json.loads(line)['event'] for line in (out/'events.jsonl').read_text().splitlines()]
        if mode=='ok':assert events==['WAITING_FOR_IDENTIFIED_ROM','ROM_SEEN','ROM_IDENTIFIED_WRITING','WRITE_VERIFIED']
        else:assert events[-1]=='PROGRAMME_FAILED' and 'WRITE_VERIFIED' not in events
        if mode in ['wrong-uid','retired']:assert 'write' not in calls and 'ROM_IDENTIFIED_WRITING' not in events
        assert 'Release USER' not in text.getvalue() and 'then RESET' not in text.getvalue()
print('K1_PROGRAMME_LIVE_EVENTS_HOST=PASS (success, wrong UID, verify failure, retired image)')
