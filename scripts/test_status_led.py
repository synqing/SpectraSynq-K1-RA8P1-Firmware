#!/usr/bin/env python3
"""Host tests for Titan status LED patterns, GPIO inversion and lifecycle."""
import json
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT
with tempfile.TemporaryDirectory(prefix='k1-status-led-') as temp:
    directory=Path(temp)
    command=['cc','-std=c11','-O2','-DK1_STATUS_GPIO_STUB=1','-DK1_LED2_PHY_STUB=1',
             '-I'+str(ROOT/'platform/ra8p1'),
             str(ROOT/'tests/host/test_status_led.c'),
             str(ROOT/'platform/ra8p1/k1_status_led.c'),
             str(ROOT/'platform/ra8p1/titan_status_gpio.c'),
             str(ROOT/'platform/ra8p1/titan_led2_phy.c'),
             '-o',str(directory/'test')]
    run(command)
    document=json.loads(run([str(directory/'test')]))
    assert document['schema']==1
    led2=document['led2']
    assert isinstance(led2['ok'],bool)
    assert isinstance(led2['attempt'],int)
    assert isinstance(led2['dwt_ok'],bool)
    assert isinstance(led2['dwt_hz'],int)
    assert led2['txc_state']=='unverified'
    assert isinstance(led2['trace']['valid'],bool)
    print('K1_STATUS_LED=PASS')
