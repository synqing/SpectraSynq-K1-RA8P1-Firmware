"""Versioned five-Waveform selection and source-consumed controls."""
import struct
from pathlib import Path
import pytest
import emotiscope_control as c
from test_inhouse_codec import state

GOLDEN=bytes.fromhex((Path(__file__).resolve().parents[3] / 'harness/tests/waveform-v8-golden.wire').read_text())

def full_state(version=8,mode=8,caps=511,engine=2):
    raw=state(engine,mode,caps)
    if version>=7:raw.extend(struct.pack('<f',.05))
    if version>=8:raw.extend(struct.pack('<3f',2.4,1.,1.))
    struct.pack_into('<I',raw,4,version)
    return raw


def test_v8_golden_extra_mask_and_legacy_rejection():
    fields={'current_mode':8,'board':1,'inhouse_mood':.05,'inhouse_sensitivity':2.4,'inhouse_square_iter':1.,'inhouse_injection_photons':1.}
    wire=c.encode_patch(0x22,0x11,0x33,fields,state_version=8,board_capabilities=511)
    assert wire==GOLDEN
    status,parsed=c.parse(GOLDEN,state_version=8)
    assert status==0 and parsed['mask']==(1<<8)|(1<<23)|(15<<63)
    assert parsed['fields'][65]==pytest.approx(2.4)
    for version in range(1,8):assert c.parse(GOLDEN,state_version=version)[0]==c.STATUS_INVALID
    corrupt=bytearray(GOLDEN);struct.pack_into('<I',corrupt,28,15)
    assert c.parse(corrupt,state_version=8)[0]==c.STATUS_INVALID
    corrupt=bytearray(GOLDEN);struct.pack_into('<I',corrupt,0,c.INHOUSE_PATCH_SUB)
    assert c.parse(corrupt,state_version=8)[0]==c.STATUS_INVALID


@pytest.mark.parametrize('mode',[32,18,8,7,11])
def test_exact_mode_capability_and_state_pairing(mode):
    caps=128 if mode==32 else 256
    minimum=6 if mode==32 else 8
    for version in range(1,9):
        if version<minimum:
            with pytest.raises(ValueError):c.encode_patch(1,2,3,{'current_mode':mode},state_version=version,board_capabilities=0)
        else:
            assert c.parse(c.encode_patch(1,2,3,{'current_mode':mode},state_version=version,board_capabilities=caps),state_version=version)[0]==0
    assert c.decode_state(full_state(mode=mode))['renderer_family']=='inhouse_waveform'
    for badmode in (9,10,12,17,19,31,33):
        with pytest.raises(ValueError):c.encode_patch(1,2,3,{'current_mode':badmode},state_version=8,board_capabilities=511)


def test_v7_retains_original_capability_mask():
    old=c.decode_state(full_state(version=7,mode=32,caps=255))
    assert old['latest_state_version']==7 and old['supported_modes']==list(range(7))+[32]
    assert not old['waveform_family_supported']
    with pytest.raises(ValueError):c.decode_state(full_state(version=7,mode=32,caps=511))
    for mode in (18,8,7,11):
        with pytest.raises(ValueError):c.decode_state(full_state(version=7,mode=mode))
        with pytest.raises(ValueError):c.encode_patch(1,2,3,{'current_mode':mode},state_version=7,board_capabilities=511)
    with pytest.raises(ValueError):c.decode_state(full_state(version=6,mode=32,caps=511))
    new=c.decode_state(full_state());assert new['supported_modes']==list(range(7))+[32,18,8,7,11]
    assert 'speed' not in new['active_controls'] and 'inhouse_sensitivity' in new['active_controls']
    for mode in (18,32):assert 'inhouse_square_iter' not in c.decode_state(full_state(mode=mode))['active_controls']


@pytest.mark.parametrize('field,lower,upper',[(65,.1,20.),(66,0.,8.),(67,.05,1.)])
def test_source_ranges_and_older_profiles(field,lower,upper):
    for value in (lower,upper):assert c.parse(c.encode_patch(1,2,3,{field:value},state_version=8,board_capabilities=511),state_version=8)[0]==0
    for value in (lower-.01,upper+.01,float('nan'),float('inf')):
        with pytest.raises(ValueError):c.encode_patch(1,2,3,{field:value},state_version=8,board_capabilities=511)
    with pytest.raises(ValueError):c.encode_patch(1,2,3,{field:lower},state_version=7,board_capabilities=511)
    with pytest.raises(ValueError):c.encode_patch(1,2,3,{field:lower},state_version=8,board_capabilities=255)


def test_mood_on_family_only_capability():
    wire=c.encode_patch(1,2,3,{'current_mode':8,'inhouse_mood':.25},state_version=8,board_capabilities=256)
    assert c.parse(wire,state_version=8)[0]==0


def test_original_v7_client_accepts_unchanged_reply():
    import subprocess
    import types
    original=types.ModuleType('original_v7_codec')
    root=Path(__file__).resolve().parents[3]
    source=subprocess.check_output(['/usr/bin/git','show','1241779aa9e43424de364f1bdba5ea18b082d3c0:tools/serial-studio/emotiscope_control.py'],cwd=root,text=True)
    exec(compile(source,'original_v7_codec','exec'),original.__dict__)
    for engine,mode in ((1,6),(2,32)):
        wire=full_state(version=7,mode=mode,caps=255,engine=engine)
        old=original.decode_state(wire);new=c.decode_state(wire)
        for key in ('version','mode','renderer_family','board_capabilities','inhouse_mood'):
            assert old[key]==new[key]


@pytest.mark.parametrize('mode',list(range(7))+[32,18,8,7,11])
def test_complete_storage_patch_keeps_inactive_controls_inactive(mode):
    values={'current_mode':mode,'inhouse_mood':.05,'inhouse_sensitivity':2.4,'inhouse_square_iter':1.,'inhouse_injection_photons':1.}
    packet=c.encode_patch(1,2,3,values,state_version=8,board_capabilities=511)
    status,parsed=c.parse(packet,state_version=8)
    assert status==0 and parsed['fields'][9]==mode
    readback=c.decode_state(full_state(mode=mode,engine=1 if mode<7 else 2))
    if mode>=7:
        active=readback['active_controls']
        if mode!=8:assert 'inhouse_sensitivity' not in active and 'inhouse_injection_photons' not in active
        if mode not in (7,8,11):assert 'inhouse_square_iter' not in active
