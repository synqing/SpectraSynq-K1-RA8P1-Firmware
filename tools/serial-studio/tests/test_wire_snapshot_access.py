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

@pytest.mark.parametrize('selector',[b'',struct.pack('<I',0),struct.pack('<I',1)])
def test_exact_board_selectors(selector):
    serial=FakeSerial();serial.feed_response(0,1,0,payload())
    assert Transport(serial,allowed={19},campaign=True).transact(19,selector)['ok']

@pytest.mark.parametrize('selector',[b'\1',b'\0'*3,b'\0'*5,struct.pack('<I',2),struct.pack('>I',1)])
def test_malformed_board_selector_never_writes(selector):
    serial=FakeSerial()
    with pytest.raises(ObserveDenied):Transport(serial,allowed={19},campaign=True).transact(19,selector)
    assert not serial.tx

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

@pytest.mark.parametrize('returned_board',[0,1])
def test_top_snapshot_is_separate_and_checks_firmware_board(tmp_path,returned_board):
    import json
    from titan_broker import Broker
    b=Broker.__new__(Broker);b.evidence=tmp_path;b.transport=object();b.replay_only=False
    b.emit_deadline=None;b.continuous_output=True;b.control_enabled=True;b.bound={'accepted':True}
    b.last_palette={'emit_enabled':True};b.errors=[]
    raw=bytearray(payload());struct.pack_into('<I',raw,8,returned_board);struct.pack_into('<I',raw,40,1<<31)
    calls=[];b.transact=lambda op,body:(calls.append((op,body)) or {'ok':True,'body':raw})
    b.apply_emit=lambda enabled:calls.append(('emit',enabled))
    (tmp_path/'wire-snapshot.bin').write_bytes(b'preserve bottom')
    (tmp_path/'WIRE_SNAPSHOT_REQUEST').write_text('bounded-top')
    b._service_emit_requests()
    assert calls==[(19,struct.pack('<I',1))]
    assert (tmp_path/'wire-snapshot.bin').read_bytes()==b'preserve bottom'
    assert (tmp_path/'wire-snapshot-top.bin').exists()==bool(returned_board)
    if returned_board:
        record=json.loads((tmp_path/'wire-snapshot-top.json').read_text())
        assert record['score']['output_channel']==1 and record['score']['pass_wire'] is None

@pytest.mark.parametrize('accepted,valid',[(True,True),(True,False),(False,True)])
def test_bounded_mir_snapshot_obeys_identity_and_requires_json_object(tmp_path,accepted,valid):
    import json
    from titan_broker import Broker
    b=Broker.__new__(Broker);b.evidence=tmp_path;b.transport=object();b.replay_only=False
    b.emit_deadline=None;b.continuous_output=True;b.control_enabled=True;b.bound={'accepted':accepted}
    b.last_palette={'emit_enabled':True};b.errors=[]
    calls=[];b.transact=lambda op,body:(calls.append((op,body)) or {'ok':True,'body':b'{"generation96":7}' if valid else b'[]'})
    (tmp_path/'MIR_SNAPSHOT_REQUEST').touch();b._service_emit_requests();b._service_emit_requests()
    assert calls==([(21,b'')] if accepted else [])
    assert (tmp_path/'mir-snapshot.json').exists()==(accepted and valid)
    if accepted and valid:assert json.loads((tmp_path/'mir-snapshot.json').read_text())['body']['generation96']==7


@pytest.mark.parametrize('mismatch',[None,'uid','build','schema_sha256'])
def test_live_candidate_snapshot_uses_same_identity_admission(tmp_path,monkeypatch,mismatch):
    import json
    from titan_broker import Broker
    monkeypatch.setenv('TITAN_LIVE_CONTROL_ADMITTED','1')
    identity={'uid':'target','build':'image','schema_sha256':'schema'}
    record=dict(identity,ops=['emotiscope_get','emotiscope_patch'])
    if mismatch:record[mismatch]='wrong'
    (tmp_path/'accepted-candidate.json').write_text(json.dumps(record))
    b=Broker.__new__(Broker)
    b.evidence=tmp_path;b.transport=object();b.replay_only=False
    b.emit_deadline=None;b.continuous_output=True;b.control_enabled=True
    b.bound=dict(identity,accepted=False,identified=True)
    b.last_palette={'emit_enabled':True};b.errors=[]
    calls=[]
    b.transact=lambda op,body:(calls.append((op,body)) or {'ok':True,'body':payload()})
    b.apply_emit=lambda enabled:calls.append(('emit',enabled))
    (tmp_path/'WIRE_SNAPSHOT_REQUEST').write_text('bounded')
    b._service_emit_requests()
    assert calls==([] if mismatch else [(19,b'')])
    assert (tmp_path/'wire-snapshot.bin').exists()==(mismatch is None)


@pytest.mark.parametrize("case", [
    "checkpoint", "candidate", "missing_capability", "false_capability",
    "string_capability", "missing_opcode", "wrong_opcode", "fixture_runtime",
    "unsupported_protocol", "wrong_uid", "wrong_build", "wrong_schema",
    "observe_only", "candidate_wrong_build",
])
def test_real_attach_mir_contract_and_identity_before_transport(tmp_path, case):
    """Exercise attach admission and the file trigger through the real Transport."""
    import json
    from test_live_control_path import _checkpoint, _usb
    from titan_live_device import LiveDeviceSerial
    from titan_broker import Broker

    class Device(LiveDeviceSerial):
        mir_calls = 0

        def _info_body(self):
            info = json.loads(super()._info_body())
            # Exact MIR declaration in fixture_app.cpp INFO and the observed
            # 037dd22f INFO: protocol 1, live, legacy_mir 21, capability true.
            info.update(opcodes={"legacy_mir": 21},
                        capabilities={"legacy_mir_json": True})
            if case == "missing_capability":
                info.pop("capabilities")
            elif case == "false_capability":
                info["capabilities"]["legacy_mir_json"] = False
            elif case == "string_capability":
                info["capabilities"]["legacy_mir_json"] = "true"
            elif case == "missing_opcode":
                info.pop("opcodes")
            elif case == "wrong_opcode":
                info["opcodes"]["legacy_mir"] = 22
            elif case == "fixture_runtime":
                info["runtime_kind"] = "fixture"
            elif case == "unsupported_protocol":
                info["protocol"] = 2
            elif case == "wrong_uid":
                info["uid"] = "wrong"
            return json.dumps(info).encode()

        def _handle(self, op, req, body):
            if op == 21:
                self.mir_calls += 1
                assert body == b""
                self._respond(0, req, b'{"generation96":7}')
            else:
                super()._handle(op, req, body)

    device = Device()
    evidence = _checkpoint(tmp_path)
    if case in ("wrong_build", "wrong_schema"):
        setattr(device, "build" if case == "wrong_build" else "schema", "wrong")
    if case.startswith("candidate"):
        (evidence / "accepted-checkpoint.json").unlink()
        (evidence / "accepted-candidate.json").write_text(json.dumps({
            "uid": device.uid,
            "build": "wrong" if case == "candidate_wrong_build" else device.build,
            "schema_sha256": device.schema,
            "ops": ["emotiscope_get", "emotiscope_patch"],
        }))
    broker = Broker(evidence, backend=device, control_enabled=case != "observe_only")
    try:
        if case in ("unsupported_protocol", "wrong_uid"):
            with pytest.raises(RuntimeError, match="identity rejected"):
                broker.attach(_usb(tmp_path))
        else:
            broker.attach(_usb(tmp_path))
            request = evidence / "MIR_SNAPSHOT_REQUEST"
            request.touch()
            broker._service_emit_requests()
            broker._service_emit_requests()
        expected = case in ("checkpoint", "candidate")
        assert device.mir_calls == int(expected)
        assert (evidence / "mir-snapshot.json").exists() == expected
        if expected:
            record = json.loads((evidence / "mir-snapshot.json").read_text())
            assert record["body"] == {"generation96": 7}
            assert record["identity"]["uid"] == device.uid
            assert not request.exists()
            assert not (evidence / "DIAGNOSTIC_FAULT").exists()
            before = len(device.tx)
            with pytest.raises(ObserveDenied, match="payload not admitted"):
                broker.transact(21, b"x")
            assert len(device.tx) == before
    finally:
        broker.shutdown()
