"""The v7 extension retains broker admission and rejects malformed packets pre-write."""
import struct
import threading
import pytest
import emotiscope_control as donor
from titan_transport import Transport,FakeSerial,ObserveDenied

def request():
    return donor.encode_patch(7,2,3,{'board':1,'current_mode':32,'inhouse_mood':.25},state_version=7,board_capabilities=255)

def test_valid_v7_payload_has_existing_campaign_transport():
    serial=FakeSerial();serial.feed_response(0,1,0,b'ack')
    assert Transport(serial,allowed={25},campaign=True).transact(25,request())['body']==b'ack'

@pytest.mark.parametrize('mutation',['truncate','oversize','reserved','duplicate','nan'])
def test_malformed_v7_is_atomic_before_serial(mutation):
    wire=bytearray(request())
    if mutation=='truncate':wire.pop()
    elif mutation=='oversize':wire.extend(bytes(544))
    elif mutation=='reserved':struct.pack_into('<I',wire,28,1)
    elif mutation=='duplicate':wire[-8:-6]=wire[-16:-14]
    else:struct.pack_into('<I',wire,len(wire)-4,0x7fc00000)
    serial=FakeSerial()
    with pytest.raises(ObserveDenied):Transport(serial,allowed={25},campaign=True).transact(25,wire)
    assert not serial.tx

@pytest.mark.parametrize('sub',[donor.PATCH_SUB,donor.EXTENDED_PATCH_SUB,donor.INHOUSE_PATCH_SUB,donor.WAVEFORMS_PATCH_SUB])
def test_all_patch_generations_preserve_broker_observe_only_hold(tmp_path,sub):
    from titan_broker import Broker
    from titan_control_server import RestrictedControlError
    class NoWrite:
        def transact(self,*args,**kwargs):raise AssertionError('must not reach serial')
    broker=Broker.__new__(Broker);broker.transport=NoWrite();broker._sched=threading.Lock()
    broker.evidence=tmp_path;broker.control_enabled=False
    with pytest.raises(RestrictedControlError):broker.transact(25,struct.pack('<I',sub))

def test_v7_ack_length_is_admitted_but_invalid_reservation_is_not():
    from test_inhouse_codec import state
    from titan_control_server import _decode_donor_body
    raw=state(2,32);raw.extend(struct.pack('<f',.25));struct.pack_into('<I',raw,4,7)
    assert _decode_donor_body(raw)['inhouse_mood']==.25
    assert _decode_donor_body(raw[:-1]) is None
    raw[420]=1
    assert _decode_donor_body(raw) is None

def test_v7_control_server_negotiates_and_preserves_atomic_rejection(tmp_path,monkeypatch):
    import json
    from test_inhouse_codec import state
    from test_titan_control_server import UID,BUILD,SCHEMA,_bound
    from titan_control_server import RestrictedControlPlane
    monkeypatch.setenv('TITAN_LIVE_CONTROL_ADMITTED','1')
    (tmp_path/'accepted-checkpoint.json').write_text(json.dumps(dict(uid=UID,build=BUILD,schema_sha256=SCHEMA,ops=['emotiscope_patch'])))
    raw=state(2,32);raw.extend(struct.pack('<f',.25));struct.pack_into('<I',raw,4,7);struct.pack_into('<I',raw,12,2)
    calls=[]
    plane=RestrictedControlPlane(transact=lambda op,wire:(calls.append((op,wire)) or {'ok':True,'status':0,'body':bytes(raw)}),evidence=tmp_path,bound=_bound())
    session=plane.handle({'op':'hello'})['session']
    request=dict(op='emotiscope_get',session=session,request_id=7,boot_id=2,revision=0,state_version=7)
    got=plane.handle(request)
    assert got['ok'] and got['state']['inhouse_mood']==.25
    assert got['state']['renderer_family']=='inhouse_waveform' and got['state']['latest_state_version']==7
    patch=dict(request,op='emotiscope_patch',fields={'board':1,'inhouse_mood':.4})
    assert plane.handle(patch)['ok']
    assert int.from_bytes(calls[-1][1][:4],'little')==donor.INHOUSE_PATCH_SUB
    count=len(calls)
    assert not plane.handle(dict(patch,fields={'board':1,'inhouse_mood':float('nan')}))['ok']
    assert len(calls)==count


def test_v8_control_server_negotiates_and_routes_actual_extra_field(tmp_path, monkeypatch):
    import json
    from test_inhouse_codec import state
    from test_titan_control_server import UID, BUILD, SCHEMA, _bound
    from titan_control_server import RestrictedControlPlane
    monkeypatch.setenv("TITAN_LIVE_CONTROL_ADMITTED", "1")
    (tmp_path / "accepted-checkpoint.json").write_text(json.dumps(dict(uid=UID, build=BUILD, schema_sha256=SCHEMA, ops=["emotiscope_patch"])))
    raw=state(2,32)
    raw.extend(struct.pack("<ffff", .05, 2.4, 1., 1.))
    struct.pack_into("<I",raw,4,8)
    struct.pack_into("<I",raw,12,2)
    struct.pack_into("<I",raw,164,511)
    calls=[]
    plane=RestrictedControlPlane(transact=lambda op,wire:(calls.append((op,wire)) or {"ok":True,"status":0,"body":bytes(raw)}),evidence=tmp_path,bound=_bound())
    session=plane.handle({"op":"hello"})["session"]
    request=dict(op="emotiscope_get",session=session,request_id=8,boot_id=2,revision=0,state_version=8)
    got=plane.handle(request)
    assert got["ok"] and got["state"]["supported_modes"]==list(range(7))+[32,18,8,7,11]
    patch=dict(request,op="emotiscope_patch",fields={"board":1,"current_mode":8,"inhouse_sensitivity":3.})
    assert plane.handle(patch)["ok"]
    assert int.from_bytes(calls[-1][1][:4],"little")==donor.WAVEFORMS_PATCH_SUB
    assert donor.parse(calls[-1][1],state_version=8)[0]==donor.STATUS_OK


def test_v8_public_readback_preserves_complete_source_configuration():
    from test_inhouse_codec import state
    from titan_control_server import _decode_donor_body
    raw=state(2,32);raw.extend(struct.pack('<ffff',.05,2.4,1.,.7))
    struct.pack_into('<I',raw,4,8);struct.pack_into('<I',raw,164,511)
    actual=_decode_donor_body(raw)
    assert actual['inhouse_sensitivity']==pytest.approx(2.4)
    assert actual['inhouse_square_iter']==1.
    assert actual['inhouse_injection_photons']==pytest.approx(.7)
    assert actual['waveform_family_supported'] is True
