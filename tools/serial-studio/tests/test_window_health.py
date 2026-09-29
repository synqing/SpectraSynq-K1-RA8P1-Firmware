import copy
import sys
from pathlib import Path
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from titan_window_health import WindowHealth,HealthError

def row():
    return dict(identity=dict(uid='u',build='b',schema_sha256='s',accepted=True,boot_id=1),
        epoch=1,pid=1,quiesced=False,topic_mono=dict(identity=10,pdm=10,palette=10),
        palette=dict(pair_fault=0,lane_a0_fault=0,lane_a1_fault=0,emit_errors=0,
            pair_completions=30,submitted_generation=31,completed_generation=30,
            brightness=100,mode_a=8,emit_enabled=True,lane_a0_dma=0,lane_a1_dma=0),
        pdm=dict(ap_hops=100,rate_locked=1,running=True,asrc_starved=0,pair_skew_drops=0,
            pair_epoch_drops=0,push_rejected=0,stale_discards=0,
            lanes=[dict(error_callbacks=0,overflow_events=0,drop_events=0,recovery_count=0) for _ in range(2)]))

def test_transient_flags_are_irrelevant():
    h=WindowHealth('u','b','s');r=row();h.check(r,10,True)
    r['pdm']['ap_hops']+=1;r['palette']['pair_completions']+=30
    h.check(r,11,True)

@pytest.mark.parametrize('field',['rate_locked','ap_hops','lanes'])
def test_missing_audio_invalid(field):
    r=row();del r['pdm'][field]
    with pytest.raises(HealthError):WindowHealth('u','b','s').check(r,10)

@pytest.mark.parametrize('field',['emit_enabled','pair_fault','brightness','completed_generation'])
def test_missing_palette_invalid(field):
    r=row();del r['palette'][field]
    with pytest.raises(HealthError):WindowHealth('u','b','s').check(r,10)

def test_frozen_audio_rejected_even_with_fresh_packets():
    h=WindowHealth('u','b','s');r=row();h.check(r,10)
    r['topic_mono']=dict(identity=16,pdm=16,palette=16)
    with pytest.raises(HealthError,match='audio stalled'):h.check(r,16)

def test_error_increment_and_reboot():
    for change in ('error','boot'):
        h=WindowHealth('u','b','s');r=row();h.check(r,10)
        if change=='error':r['pdm']['lanes'][0]['error_callbacks']+=1
        else:r['identity']['boot_id']+=1
        with pytest.raises(HealthError):h.check(r,11)

def test_stale_and_wrong_build():
    for change in ('stale','build'):
        r=row()
        if change=='stale':r['topic_mono']['pdm']=0
        else:r['identity']['build']='other'
        with pytest.raises(HealthError):WindowHealth('u','b','s').check(r,10)

def test_positive_fault_not_hidden_by_missing_identity():
    r=row();r['identity']={};r['palette']['pair_fault']=1
    with pytest.raises(HealthError) as e:WindowHealth('u','b','s').check(r,10)
    assert e.value.verdict=='FAIL'


def test_out_of_range_brightness_rejected():
    r=row();r['palette']['brightness']=256
    with pytest.raises(HealthError,match='brightness invalid'):
        WindowHealth('u','b','s').check(r,10)
