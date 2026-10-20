"""Performance opcode admission and codec: exercise the real control server."""
import json
import struct
import threading
from types import SimpleNamespace
import pytest
import emotiscope_control as donor
import live_protocol as p
from titan_control_server import RestrictedControlPlane, RestrictedControlError
from titan_broker import Broker


def test_performance_pack_all_fields_and_rejects():
    fields={name:(lo+hi)/2 for name,(_,lo,hi) in list(p.PERFORMANCE_FIELDS.items())[:8]}
    wire=p.pack_performance(7,fields,91,4,palette=36)
    assert len(wire)==88 and struct.unpack_from('<IIIIHHI',wire)==(10,36,7,91,8,0,4)
    actual={}
    for off in range(24,len(wire),8):
        id,kind,value=struct.unpack_from('<HHf',wire,off);actual[id]=value;assert kind==1
    assert len(actual)==8
    assert p._u32(p.pack_performance(7,{'focus_a.level_gain':1.5},92,4,audition=True),0)==11
    with pytest.raises(ValueError):p.pack_performance(7,{"visual_a.chroma":.5},92,4,audition=True)
    for field,value in [('emit_on',1),('focus_a.onset_gain',1),('visual_b.chroma',.5),('visual_a.chroma',float('nan')),('visual_a.chroma',2),('visual_a.chroma',True)]:
        with pytest.raises((ValueError,TypeError)):p.pack_performance(7,{field:value},91,4)
    for revision,rid,boot,palette in [(-1,1,1,None),(0,0,1,None),(0,1,0,None),(0,1,1,0),(False,1,1,None)]:
        with pytest.raises(ValueError):p.pack_performance(revision,{'visual_a.chroma':.1},rid,boot,palette)


def test_server_applied_focus_values_and_scene_scope(tmp_path,monkeypatch):
    monkeypatch.setenv('TITAN_LIVE_CONTROL_ADMITTED','1')
    bound=dict(uid='545433931bd25436593630352d068363',build='test',schema_sha256=p.SCHEMA_SHA256)
    scope=['focus_a.level_gain','visual_a.vp_waveform_shift_rate']
    record={**bound,'ops':['patch','performance','get_config'],'fields':scope}
    file=tmp_path/'accepted-candidate.json';file.write_text(json.dumps(record))
    cfg=p.unpack_config(p.pack_config(dict(version=2,revision=8,boot_id=4,mode_a=8,mode_b=8,emit_on=1,last_patch_request_id=91,focus_a=[1.5]+[1.]*14,visual_a={'vp_waveform_shift_rate':30.},palette_a=36)))
    calls=[]
    def transact(op,wire):
        calls.append((op,wire));return dict(ok=True,body=p.pack_config(cfg))
    plane=RestrictedControlPlane(transact=transact,evidence=tmp_path,bound=bound)
    request=dict(op='performance',session=plane.handle({'op':'hello'})['session'],revision=7,boot_id=4,request_id=91,fields={'focus_a.level_gain':1.5,'visual_a.vp_waveform_shift_rate':30.})
    reply=plane.handle(request);assert reply['ok'] and reply['applied']==request['fields']
    assert p._u32(calls[-1][1],0)==9
    request['palette']=36
    assert plane.handle(request)['admit_reason']=='field_scope';assert len(calls)==1
    record['fields'].append('scene.palette_a');file.write_text(json.dumps(record))
    assert plane.handle(request)['ok'];assert p._u32(calls[-1][1],0)==10
    record['ops'].remove('performance');file.write_text(json.dumps(record))
    assert plane.handle(request)['admit_reason']=='performance_not_admitted'
    assert len(calls)==2


@pytest.mark.parametrize('sub',[8,9,10,11,12])
def test_final_broker_boundary_rechecks_admission(sub):
    fake=SimpleNamespace(transport=SimpleNamespace(transact=lambda *a,**kw:pytest.fail('unadmitted wire write')),
                         _sched=threading.Lock(),quiesced=lambda:True)
    with pytest.raises(RestrictedControlError,match='paused'):
        Broker.transact(fake,25,struct.pack('<I',sub))


def test_donor_patch_is_gated_and_get_is_not():
    calls=[]
    def transport_transact(op, payload=b'', timeout=2.0):
        calls.append((op, bytes(payload)))
        return {'ok': True, 'status': 0, 'body': b''}
    paused=SimpleNamespace(transport=SimpleNamespace(transact=transport_transact),
                           _sched=threading.Lock(), quiesced=lambda: True)
    with pytest.raises(RestrictedControlError, match='paused'):
        Broker.transact(paused, 25, donor.encode_patch(1, 0x11, 0, {'brightness': 0.5}))
    assert calls == []
    Broker.transact(paused, 25, donor.encode_get(4, 0x11))
    assert calls == [(25, donor.encode_get(4, 0x11))]


def test_selection_codec_and_finite_modes():
    for mode in p.CONTRACT['performance']['selection']['modes']:
        assert struct.unpack('<IIIIII',p.pack_selection(7,91,4,mode,36)) == (12,7,91,4,mode,36)
    for mode in [0,1,2,4,255,True]:
        with pytest.raises(ValueError):p.pack_selection(7,91,4,mode,36)
    with pytest.raises(ValueError):p.pack_selection(7,0,4,8,36)

def test_selection_server_scope_and_ack(tmp_path, monkeypatch):
    monkeypatch.setenv('TITAN_LIVE_CONTROL_ADMITTED','1')
    bound=dict(uid='545433931bd25436593630352d068363',build='test',schema_sha256=p.SCHEMA_SHA256)
    record={**bound,'ops':['patch','selection','get_config'],'fields':['scene.palette_a','scene.mode']}
    (tmp_path/'accepted-candidate.json').write_text(json.dumps(record))
    cfg=dict(version=2,revision=8,boot_id=4,mode_a=7,mode_b=7,emit_on=1,last_patch_request_id=91,palette_a=36)
    calls=[]
    def transact(op,wire):
        calls.append((op,wire));return dict(ok=True,body=p.pack_config(cfg))
    plane=RestrictedControlPlane(transact=transact,evidence=tmp_path,bound=bound)
    req=dict(op='selection',session=plane.handle({'op':'hello'})['session'],revision=7,boot_id=4,request_id=91,mode=7,palette=36)
    reply=plane.handle(req);assert reply['ok'] and reply['mode_a']==7 and reply['mode_b']==7 and reply['emit_on']==1
    assert len(calls)==1 and calls[0][1]==p.pack_selection(7,91,4,7,36)
    record['fields'].remove('scene.mode');(tmp_path/'accepted-candidate.json').write_text(json.dumps(record))
    assert plane.handle(req)['admit_reason']=='field_scope';assert len(calls)==1


def test_cross_effect_scene_has_bounded_scalar_payload():
    fields={'focus_a.kick_gain':1.5,'focus_a.snare_gain':.5}
    wire=p.pack_selection(7,91,4,26,36,fields)
    assert len(wire)==40 and wire[24:]==p.pack_performance(7,fields,91,4)[20:]
    with pytest.raises(ValueError):p.pack_selection(7,91,4,26,36,{name:lo for name,(_,lo,hi) in p.PERFORMANCE_FIELDS.items()})
    for name in ['novelty','kick','snare','hihat','transient']:
        assert p._u32(p.pack_performance(7,{'focus_a.'+name+'_gain':1.5},91,4,audition=True),0)==11


def test_selection_scene_reads_back_focus_and_visual_values(tmp_path, monkeypatch):
    monkeypatch.setenv('TITAN_LIVE_CONTROL_ADMITTED','1')
    bound=dict(uid='545433931bd25436593630352d068363',build='test',schema_sha256=p.SCHEMA_SHA256)
    fields={'focus_a.kick_gain':1.5,'visual_a.bulb_opacity':.5}
    record={**bound,'ops':['patch','selection'],'fields':list(fields)+['scene.palette_a','scene.mode']}
    (tmp_path/'accepted-candidate.json').write_text(json.dumps(record))
    focus=[1.]*15;focus[10]=1.5
    cfg=dict(version=2,revision=8,boot_id=4,mode_a=26,mode_b=26,emit_on=1,last_patch_request_id=91,palette_a=36,focus_a=focus,visual_a={'bulb_opacity':.5})
    plane=RestrictedControlPlane(transact=lambda *args:dict(ok=True,body=p.pack_config(cfg)),evidence=tmp_path,bound=bound)
    req=dict(op='selection',session=plane.handle({'op':'hello'})['session'],revision=7,boot_id=4,request_id=91,mode=26,palette=36,fields=fields)
    reply=plane.handle(req)
    assert reply['ok'] and reply['applied']==fields
    record['fields'].remove('focus_a.kick_gain')
    (tmp_path/'accepted-candidate.json').write_text(json.dumps(record))
    assert plane.handle(req)['admit_reason']=='field_scope'
