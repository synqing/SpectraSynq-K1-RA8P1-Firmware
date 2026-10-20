import struct
import pytest
from titan_register_snapshot import decode, FIELDS

def packet():
    words=list(range(len(FIELDS)));words[:2]=[2,len(FIELDS)*4]
    return struct.pack("<"+"I"*len(words),*words)

def test_physical_compare_order_and_lanes():
    d=decode(packet())
    assert d['gpt_a_ccre']+1==d['gpt_a_ccrd']
    assert d['gpt_b_ccrf']>d['gpt_a_ccrf']
    assert d['dma3_activation']+1==d['elc_0']

def test_legacy_and_clock_dividers():
    from titan_register_snapshot import V1_FIELDS
    values=[1,len(V1_FIELDS)*4]+[0]*(len(V1_FIELDS)-2)
    old=decode(struct.pack("<"+"I"*len(values),*values))
    assert 'pllccr2' not in old
    current=decode(packet())
    assert current['pllccr2']+1 == current['pll2ccr2']
    assert current['pll2ccr2']+1 == current['gptckdivcr']

@pytest.mark.parametrize('raw',[b'',packet()[:-4],packet()+bytes(4),bytes(len(packet()))])
def test_invalid_layout(raw):
    with pytest.raises(ValueError):decode(raw)

def test_broker_snapshot_is_read_only_and_bound(tmp_path):
    from titan_broker import Broker
    b=Broker.__new__(Broker)
    b.evidence=tmp_path;b.transport=object();b.replay_only=False
    b.emit_deadline=None;b.control_enabled=True;b.bound={'accepted':True,'uid':'fixture'}
    b.last_palette={'emit_enabled':False};b.errors=[]
    calls=[]
    def transact(op,payload):
        calls.append((op,payload));return {'ok':True,'body':packet()}
    b.transact=transact
    (tmp_path/'HWST_REQUEST').write_text('read')
    b._service_emit_requests()
    assert calls==[(17,b'HWST')]
    assert (tmp_path/'registers.bin').read_bytes()==packet()
    assert not (tmp_path/'HWST_REQUEST').exists()
    assert not (tmp_path/'DIAGNOSTIC_FAULT').exists()

@pytest.mark.parametrize('accepted,emitting',[(False,False),(True,True)])
def test_broker_refuses_unbound_or_emitting_snapshot(tmp_path,accepted,emitting):
    from titan_broker import Broker
    b=Broker.__new__(Broker)
    b.evidence=tmp_path;b.transport=object();b.replay_only=False
    b.emit_deadline=None;b.control_enabled=True;b.bound={'accepted':accepted}
    b.last_palette={'emit_enabled':emitting};b.errors=[]
    calls=[];b.transact=lambda *args:calls.append(args)
    (tmp_path/'HWST_REQUEST').write_text('read')
    b._service_emit_requests()
    assert not calls and not (tmp_path/'registers.bin').exists()
    assert (tmp_path/'DIAGNOSTIC_FAULT').exists()
