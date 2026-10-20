"""Family selection is atomic and cannot be sent to a legacy target."""
import struct
import pytest
import emotiscope_control as c


def state(engine=1,mode=6,caps=255):
    data=bytearray(424)
    struct.pack_into('<III',data,0,c.STATE_MAGIC,6,engine)
    struct.pack_into('<i',data,60,mode)
    struct.pack_into('<I',data,140,1)
    struct.pack_into('<I',data,164,caps)
    struct.pack_into('<f',data,248,40)
    struct.pack_into('<f',data,252,160)
    # Valid existing composed defaults, independent from family selection.
    struct.pack_into('<I8fI3fIIfIII',data,320,0,.08,1200.,90.,100.,.08,1800.,.55,40.,1,1.,0.,30.,0,0,0.,0,0,0)
    return data


def test_supported_family_atomic_selection():
    wire=c.encode_patch(1,2,3,{'current_mode':32,'board':1},state_version=6,board_capabilities=255)
    assert c.parse(wire,state_version=6)[0]==c.STATUS_OK
    assert c.parse(wire,state_version=4)[0]==c.STATUS_INVALID
    for version,caps in ((2,0),(4,7),(5,63),(6,127)):
        with pytest.raises(ValueError):c.encode_patch(1,2,3,{'current_mode':32},state_version=version,board_capabilities=caps)
    for mode in (7,8,11,18,31,33):
        with pytest.raises(ValueError):c.encode_patch(1,2,3,{'current_mode':mode},state_version=6,board_capabilities=255)


def test_readback_family_is_truthful():
    donor=c.decode_state(state());assert donor['renderer_family']=='emotiscope';assert donor['supported_modes']==[0,1,2,3,4,5,6,32]
    hybrid=c.decode_state(state(2,32));assert hybrid['renderer_family']=='inhouse_waveform';assert 'softness' not in hybrid['active_controls']
    for engine,mode,caps in ((0,6,255),(3,6,255),(1,32,255),(2,6,255),(2,32,127)):
        with pytest.raises(ValueError):c.decode_state(state(engine,mode,caps))
    assert 32 not in c.decode_state(state(caps=127))['supported_modes']


def test_mood_profile_requires_v7_and_valid_family():
    wire=c.encode_patch(7,2,3,{'board':1,'current_mode':32,'inhouse_mood':.25},state_version=7,board_capabilities=255)
    assert struct.unpack_from('<I',wire)[0]==c.INHOUSE_PATCH_SUB
    assert c.parse(wire,state_version=7)[0]==c.STATUS_OK
    assert c.parse(wire,state_version=6)[0]==c.STATUS_INVALID
    with pytest.raises(ValueError):c.encode_patch(7,2,3,{'inhouse_mood':.25},state_version=6,board_capabilities=255)
    raw=state(2,32);raw.extend(struct.pack('<f',.25));struct.pack_into('<I',raw,4,7)
    decoded=c.decode_state(raw);assert decoded['inhouse_mood']==.25 and decoded['inhouse_mood_supported']
    assert len(c.encode_get(8,2,state_version=6,board_capabilities=255))==16
    assert struct.unpack_from('<I',c.encode_get(8,2,state_version=7,board_capabilities=255))[0]==c.INHOUSE_GET_SUB
