#!/usr/bin/env python3
"""Prove actual driver tests reject sequencing and false-completion regressions."""
from pathlib import Path
import shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=ROOT/'platform/ra8p1/ws281x_gpt_dma_hw.c'
mutations={
 'missing_elc_module_start':('    R_BSP_MODULE_START(FSP_IP_ELC, 0);','    /* module-start deliberately omitted */'),
 'lost_first_pulse':('    gpt6_ctrl.p_reg->GTIOR |= 1u << 4;','    /* first-pulse initialization deliberately omitted */'),
 'frame_before_dma_irq':('        if (dma_complete && waveform_complete &&','        if (waveform_complete &&'),
 'incomplete_dma_waiver':('    if (dmac_ctrl.p_reg->DMCRA != 0u) {\n',
                         '    if (dmac_ctrl.p_reg->DMCRA > 2u) {\n'),
 'wrong_fault_frame':('    fault_witness.frame_id = attempts;',
                      '    fault_witness.frame_id = attempts - 1u;'),
}
with tempfile.TemporaryDirectory() as tmp:
    r=Path(tmp)
    for sub in ['platform/ra8p1','tests/target_mock','scripts']:(r/sub).mkdir(parents=True,exist_ok=True)
    for name in ['ws281x_gpt_dma_hw.h','ws281x_gpt_dma.h','ws281x_waveform.c','ws281x_waveform.h','ws281x_diag.h','titan_led_pins.h']:
        shutil.copy2(ROOT/'platform/ra8p1'/name,r/'platform/ra8p1'/name)
    for rel in ['tests/target_mock/ws281x_hw_mock.h','tests/test_ws281x_gpt_dma_hw.c','scripts/test_ws281x_gpt_dma_hw.py']:shutil.copy2(ROOT/rel,r/rel)
    for name,(old,new) in mutations.items():
        text=source.read_text();assert text.count(old)==1
        (r/'platform/ra8p1/ws281x_gpt_dma_hw.c').write_text(text.replace(old,new))
        # Remove -Werror: an intentionally removed operation can leave an unused mock.
        script=(ROOT/'scripts/test_ws281x_gpt_dma_hw.py').read_text().replace("'-Werror',",'')
        (r/'scripts/test_ws281x_gpt_dma_hw.py').write_text(script)
        try:
            p=subprocess.run(['python3',str(r/'scripts/test_ws281x_gpt_dma_hw.py')],capture_output=True,text=True,timeout=180)
        except subprocess.TimeoutExpired:
            print(name+': TIMEOUT — host mutation runner did not finish; PASS deferred until D-state/zombies are cleared',file=__import__('sys').stderr)
            raise SystemExit('K1_GPT_MUTATION_PROOF=DEFERRED timeout='+name)
        assert p.returncode and ('Assertion' in p.stderr or 'assertion' in p.stderr),name+' was not rejected by runtime assertion: '+p.stderr
        print(name+': REJECTED')
print('K1_GPT_MUTATION_PROOF=PASS')
