import struct
import pytest
from titan_transport import Transport, FakeSerial, ObserveDenied

def payload():
    return struct.pack('<16I',1,1,0,24,2,80,48,3,8,33,23,8333,924,924,99,0)+bytes(1440)

def test_transport_exact_campaign_reader():
    serial = FakeSerial()
    serial.feed_response(0,1,0,payload())
    transport = Transport(serial,allowed={19},campaign=True)
    assert transport.transact(19)['body'] == payload()

@pytest.mark.parametrize('campaign,op,body',[
    (False,19,b''),(True,19,b'x'),(True,20,b''),(True,22,b'')])
def test_unrelated_and_malformed_requests_stay_blocked(campaign,op,body):
    serial = FakeSerial()
    with pytest.raises(ObserveDenied):
        Transport(serial,allowed={op},campaign=campaign).transact(op,body)
    assert not serial.tx

@pytest.mark.parametrize('accepted,emitting,expected',[
    (True,False,True),(False,False,False),(True,True,False)])
def test_broker_read_requires_bound_and_off(tmp_path,accepted,emitting,expected):
    from titan_broker import Broker
    b=Broker.__new__(Broker)
    b.evidence=tmp_path;b.transport=object();b.replay_only=False
    b.emit_deadline=None;b.control_enabled=True;b.bound={'accepted':accepted}
    b.last_palette={'emit_enabled':emitting};b.errors=[]
    calls=[]
    def transact(op,body):
        calls.append((op,body));return {'ok':True,'body':payload()}
    b.transact=transact
    (tmp_path/'WIRE_SNAPSHOT_REQUEST').touch()
    b._service_emit_requests()
    assert calls == ([(19,b'')] if expected else [])
    assert (tmp_path/'wire-snapshot.bin').exists() == expected
    assert (tmp_path/'DIAGNOSTIC_FAULT').exists() != expected

@pytest.mark.parametrize('marker,remaining,expected',[
    ('bounded',8,True),('bounded',-1,False),('',8,False)])
def test_active_snapshot_requires_explicit_bounded_window(tmp_path,marker,remaining,expected):
    import time
    from titan_broker import Broker
    b=Broker.__new__(Broker)
    b.evidence=tmp_path;b.transport=object();b.replay_only=False
    b.emit_deadline=time.monotonic()+remaining;b.control_enabled=True;b.bound={'accepted':True}
    b.last_palette={'emit_enabled':True};b.errors=[]
    calls=[]
    b.transact=lambda op,body:(calls.append((op,body)) or {'ok':True,'body':payload()})
    b.apply_emit=lambda enabled:calls.append(('emit',enabled))
    (tmp_path/'WIRE_SNAPSHOT_REQUEST').write_text(marker)
    b._service_emit_requests()
    assert ((19,b'') in calls) == expected
    assert (tmp_path/'wire-snapshot.bin').exists() == expected

@pytest.mark.parametrize('profile,admitted',[(3,True),(4,True),(1,False),(None,False)])
def test_real_attach_contract_then_snapshot(tmp_path,profile,admitted):
    import json
    from test_live_control_path import _checkpoint, _usb
    from titan_live_device import LiveDeviceSerial
    from titan_broker import Broker
    class Device(LiveDeviceSerial):
        def _handle(self,op,req,body):
            if op == 17:
                self._respond(0,req,json.dumps({'wire_profile':profile,'emit_enabled':False}).encode())
            elif op == 19:
                self._respond(0,req,payload())
            else:
                super()._handle(op,req,body)
    device=Device()
    evidence=_checkpoint(tmp_path)
    broker=Broker(evidence,backend=device,control_enabled=True)
    try:
        broker.attach(_usb(tmp_path))
        assert (19 in broker.transport.allowed) == admitted
        if admitted:
            (evidence/'WIRE_SNAPSHOT_REQUEST').touch()
            broker._service_emit_requests()
            assert (evidence/'wire-snapshot.bin').read_bytes() == payload()
            assert not (evidence/'DIAGNOSTIC_FAULT').exists()
    finally:
        broker.shutdown()


def test_continuous_music_allows_one_bounded_observation_without_disarm(tmp_path):
    from titan_broker import Broker
    b=Broker.__new__(Broker)
    b.evidence=tmp_path;b.transport=object();b.replay_only=False
    b.emit_deadline=None;b.continuous_output=True;b.control_enabled=True
    b.bound={'accepted':True};b.last_palette={'emit_enabled':True};b.errors=[]
    calls=[]
    b.transact=lambda op,body:(calls.append((op,body)) or {'ok':True,'body':payload()})
    b.apply_emit=lambda enabled:calls.append(('emit',enabled))
    (tmp_path/'WIRE_SNAPSHOT_REQUEST').write_text('bounded')
    b._service_emit_requests()
    b._service_emit_requests()
    assert calls==[(19,b'')]
    assert (tmp_path/'wire-snapshot.bin').read_bytes()==payload()
    assert not (tmp_path/'WIRE_SNAPSHOT_REQUEST').exists()
    assert not (tmp_path/'DIAGNOSTIC_FAULT').exists()
    assert b.emit_deadline is None
