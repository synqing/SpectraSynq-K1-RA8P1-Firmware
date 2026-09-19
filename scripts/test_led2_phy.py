#!/usr/bin/env python3
from pathlib import Path
import sys
import tempfile
from run_host import run
from verify_imports import ROOT

source = (ROOT / 'platform/ra8p1/titan_led2_phy.c').read_text()
for definition in (
    '#define MDIO_PIN BSP_IO_PORT_12_PIN_12',
    '#define MDC_PIN BSP_IO_PORT_12_PIN_11',
    '#define RST_PIN BSP_IO_PORT_10_PIN_07',
):
    if definition not in source:
        raise AssertionError(f'Titan Mini HW V1.0 pin contract missing: {definition}')

if 'pin_readback != PIN_READBACK_EXPECTED' not in source or 'ta_zero_bitmap' not in source:
    raise AssertionError('LED2 diagnostics must retain MCU pin-level checks and ACK addresses')

# Exercise the production bit engine, not source-text counts of TA helpers.
# The high-level STUB test below deliberately does not cover the wire protocol.
print(run([sys.executable, str(ROOT / 'scripts/test_led2_mdio_wire.py'),
           '--source', str(ROOT / 'platform/ra8p1/titan_led2_phy.c')]), end='')

with tempfile.TemporaryDirectory(prefix='k1-led2-phy-') as temp:
    out = Path(temp) / 'test'
    run(['cc', '-std=c11', '-O2', '-DK1_LED2_PHY_STUB=1',
         '-I' + str(ROOT / 'platform/ra8p1'),
         str(ROOT / 'tests/host/test_led2_phy.c'),
         str(ROOT / 'platform/ra8p1/titan_led2_phy.c'),
         str(ROOT / 'platform/ra8p1/k1_status_led.c'),
         str(ROOT / 'platform/ra8p1/titan_status_gpio.c'),
         '-DK1_STATUS_GPIO_STUB=1',
         '-o', str(out)])
    print(run([str(out)]), end='')
