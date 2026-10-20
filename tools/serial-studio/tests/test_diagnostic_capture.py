import json
import struct
import sys
from pathlib import Path
from types import SimpleNamespace
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from titan_broker import Broker
from titan_transport import Transport, FakeSerial, ObserveDenied

@pytest.mark.parametrize('payload',[b'LBRQ',b'LBPG'+struct.pack('<I',0),b'HWST'])
def test_diagnostic_requires_campaign(payload):
    t=Transport(FakeSerial(),allowed={17})
    with pytest.raises(ObserveDenied):t.transact(17,payload)

def test_diagnostic_unknown_payload_refused():
    t=Transport(FakeSerial(),allowed={17},campaign=True)
    with pytest.raises(ObserveDenied):t.transact(17,b'XXXX')

def test_capture_raw_header_and_payload_retained(tmp_path):
    b=Broker.__new__(Broker);b.evidence=tmp_path;b.replay_only=False;b.transport=object()
    b.emit_deadline=None;b.last_palette={'emit_enabled':False};b.bound={'uid':'u','build':'b'};b.errors=[]
    words=[5,123,0,375,300000000,1,0,4,3840]+[0]*13
    raw=struct.pack('<22I',*words)+bytes(range(240))*2+struct.pack('<4H',0,2,2,0)
    calls=[]
    def transact(op,payload):
        calls.append(payload);offset=struct.unpack_from('<I',payload,4)[0]
        return dict(ok=True,body=raw[offset:offset+4096])
    b.transact=transact;(tmp_path/'LOOPBACK_DUMP').write_text('1')
    b._service_emit_requests()
    assert not b.errors
    assert (tmp_path/'loopback-raw.bin').read_bytes()==raw
    result=json.loads((tmp_path/'loopback-capture.json').read_text())
    assert result['generation']==5 and result['samples']==[0,2,2,0]
    assert result['payload_hex']==(bytes(range(240))*2).hex()
    assert result['uniform_sampling_qualified'] is False
    assert not (tmp_path/'LOOPBACK_DUMP').exists()

def test_watchdog_disarms_without_arm_request(tmp_path):
    b=Broker.__new__(Broker);b.evidence=tmp_path;b.replay_only=False;b.transport=object()
    b.emit_deadline=0;b.errors=[];calls=[]
    b.apply_emit=lambda enabled:calls.append(enabled)
    b._service_emit_requests()
    assert calls==[False]

def test_failure_clears_request_and_latches_no_retry(tmp_path):
    b=Broker.__new__(Broker);b.evidence=tmp_path;b.replay_only=False;b.transport=object()
    b.emit_deadline=None;b.errors=[];b.quiesced=lambda:False
    def fail(enable):raise RuntimeError('failed arm')
    b.apply_emit=fail;(tmp_path/'EMIT_ARM').write_text('1')
    b._service_emit_requests()
    assert not (tmp_path/'EMIT_ARM').exists()
    assert (tmp_path/'DIAGNOSTIC_FAULT').exists()


def test_register_request_exact_payload_admitted_only_in_campaign():
    fake=FakeSerial();fake.feed_response(0,1,0,b'registers')
    t=Transport(fake,allowed={17},campaign=True)
    assert t.transact(17,b'HWST')['body']==b'registers'
    for invalid in (b'HWS',b'HWSTx',b'hwst',b'HWST'+bytes(4)):
        with pytest.raises(ObserveDenied):t.transact(17,invalid)
