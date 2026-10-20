"""Continuous product operation remains distinct from timed diagnostics."""
import json
import time
from types import SimpleNamespace
import pytest
from titan_broker import Broker

@pytest.mark.parametrize('brightness',[24,100,128])
@pytest.mark.parametrize('window',[8.0,100.0])
@pytest.mark.parametrize('continuous',[False,True])
def test_arm_timer_policy_and_explicit_disarm(tmp_path,monkeypatch,continuous,window,brightness):
    monkeypatch.setattr('titan_broker.hardware_writes_admitted',lambda *a:(True,'authorised'))
    b=Broker.__new__(Broker)
    b.evidence=tmp_path;b.transport=SimpleNamespace(allowed={16})
    b.continuous_output=continuous;b.emit_window_seconds=window;b.bound={'accepted':True}
    b._control_bound=lambda x:x
    b.topic_mono={k:time.monotonic() for k in ('identity','pdm','palette')}
    b.last_metrics={'pdm_target':{'rate_locked':1}}
    b.last_palette={'mode_a':8,'mode_b':8,'pair_fault':0,'lane_a0_fault':0,'lane_a1_fault':0,'emit_enabled':False,'palette_a':33,'palette_b':43,'brightness':brightness}
    b.recorder=SimpleNamespace(put=lambda x:None)
    enabled=False
    def transact(op,payload,**kwargs):
        nonlocal enabled
        if op==16:
            import struct
            fields=struct.unpack('<10I',payload);enabled=bool(fields[5]&4)
            assert fields[6]==brightness
        return {'ok':True,'status':0,'body':json.dumps({**b.last_palette,'emit_enabled':enabled}).encode()}
    b.transact=transact
    assert b.apply_emit(True)['emit_enabled'] is True
    assert (b.emit_deadline is None)==continuous
    if not continuous: assert window-1 < b.emit_deadline-time.monotonic() <= window
    assert b.apply_emit(False)['emit_enabled'] is False
    assert b.emit_deadline is None


def test_continuous_mode_does_not_suppress_latched_fault(tmp_path):
    b=Broker.__new__(Broker);b.evidence=tmp_path;b.continuous_output=True
    b.transport=SimpleNamespace(allowed={16})
    (tmp_path/'DIAGNOSTIC_FAULT').write_text('retained hardware fault')
    with pytest.raises(RuntimeError,match='fault latched'):b.apply_emit(True)

@pytest.mark.parametrize('window', [0, 121, float('nan'), float('inf')])
def test_diagnostic_window_rejects_unbounded_duration(tmp_path, window):
    with pytest.raises(ValueError): Broker(tmp_path, emit_window_seconds=window)
