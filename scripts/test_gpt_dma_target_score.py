#!/usr/bin/env python3
"""Negative tests execute the production target decoder/scorer without serial."""
import copy,importlib.util,json,struct,sys,types
from pathlib import Path
for name,attrs in [('run_led_smoke',dict(UID='test',packet=None,read_exact=None)),('verify_imports',dict(PIN='test'))]:
    mod=types.ModuleType(name);mod.__dict__.update(attrs);sys.modules[name]=mod
spec=importlib.util.spec_from_file_location('scorer',Path(__file__).with_name('run_gpt_dma_autonomous.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
first={k:0 for k in m.HEAD};first.update(version=1,bytes=4*(12+5*len(m.FIELDS)),bits=3072,clock_hz=300000000,dma_irqs=1,stop_irqs=1,frames=1)
first['snapshots']=[dict.fromkeys(m.FIELDS,0) for _ in range(5)]
for i,s in enumerate(first['snapshots'][:4]):
    s.update(tag=i+1,attempt=1,gpt_irq_owner=1,dma_irq_owner=1,elc_enable=1,elc_a=0x1b7,elc_b=0x187,delink=0x1bd,gpt6_gtior=1<<4)
second=copy.deepcopy(first)
for k in ['dma_irqs','stop_irqs','frames']:second[k]=10
assert m.score(first,second)['latched_frames']==9
words=[second[k] for k in m.HEAD]+[s[k] for s in second['snapshots'] for k in m.FIELDS]
assert m.decode_snapshot(struct.pack('<'+'I'*len(words),*words))==second
mutations=[lambda s:s.update(dma_irqs=1),lambda s:s.update(errors=1),lambda s:s.update(first_fault=5),lambda s:s.update(frames=11),lambda s:s['snapshots'][3].update(gpt6_gtcr=1),lambda s:s['snapshots'][2].update(dmac_count=1),lambda s:s['snapshots'][0].update(elc_enable=0),lambda s:s['snapshots'][3].update(gpt_irq_owner=0),lambda s:s['snapshots'][3].update(attempt=2),lambda s:s['snapshots'][0].update(gpt6_gtior=0)]
for change in mutations:
    bad=copy.deepcopy(second);change(bad)
    try:m.score(first,bad)
    except ValueError:pass
    else:raise AssertionError('invalid target evidence accepted')
try:m.decode_snapshot(b'bad')
except ValueError:pass
else:raise AssertionError('invalid length accepted')
print('K1_GPT_TARGET_SCORER_HOST=PASS (positive + 11 negative cases)')
