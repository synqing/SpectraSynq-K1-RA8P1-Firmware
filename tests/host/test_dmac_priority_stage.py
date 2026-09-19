"""Run the actual FSP Open slice with HOST registers; no target or desktop I/O."""
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
spec = importlib.util.spec_from_file_location('colour_build', ROOT / 'scripts/build_scalar.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#define DMAC_CFG_PARAM_CHECKING_ENABLE 0
#define BSP_FEATURE_DMAC_HAS_DMCTL 1
#define R_DMA_DMCTL_PR_Pos 0
#define R_DMA_DMCTL_ERCH_Pos 4
#define DMAC_CFG_ERROR_CHANNEL_CLEAR 0
#define FSP_SUCCESS 0
#define FSP_ERR_IN_USE 8
#define DMAC_ID 0x444d4143
#define FSP_IP_DMAC 0
#define FSP_ERROR_RETURN(c,e) do {if (!(c)) return (e);} while (0)
typedef int fsp_err_t;
typedef void transfer_ctrl_t;
typedef struct {void *p_info; const void *p_extend;} transfer_cfg_t;
typedef struct {unsigned channel; void *p_callback; void *p_context;} dmac_extended_cfg_t;
typedef struct {const transfer_cfg_t *p_cfg; void *p_reg; void *p_callback;
                void *p_context; void *p_callback_memory; unsigned open;} dmac_instance_ctrl_t;
static struct {volatile uint8_t DMAST,DMCTL;} regs;
static int configured, channel;
#define R_DMA (&regs)
#define DMAC_PRV_REG(c) (&channel)
#define R_BSP_MODULE_START(ip,c) ((void)0)
static void r_dmac_config_transfer_info(dmac_instance_ctrl_t *c,void *i)
{(void)c;(void)i;++configured;}
'''

MAIN = r'''
int main(void) {
 dmac_instance_ctrl_t ctrl={0}; dmac_extended_cfg_t extend={0};
 transfer_cfg_t cfg={0,&extend};
 regs.DMAST=0;regs.DMCTL=0;
 assert(R_DMAC_Open(&ctrl,&cfg)==0);
 assert(regs.DMAST==1&&regs.DMCTL==DMAC_CFG_PRIORITY_MODE&&configured==1);
 configured=0;
 assert(R_DMAC_Open(&ctrl,&cfg)==0); /* Matching policy while another lane runs. */
 assert(regs.DMAST==1&&regs.DMCTL==DMAC_CFG_PRIORITY_MODE&&configured==1);
 configured=0;ctrl.open=0;regs.DMCTL=1-DMAC_CFG_PRIORITY_MODE;
 assert(R_DMAC_Open(&ctrl,&cfg)==FSP_ERR_IN_USE);
 assert(regs.DMAST==1&&regs.DMCTL==1-DMAC_CFG_PRIORITY_MODE&&configured==0&&ctrl.open==0);
 return 0;
}
'''


class DmacPriorityStage(unittest.TestCase):
    def test_actual_open_refuses_active_policy_change_in_both_arms(self):
        source = builder.BSP / 'FSPConfiguration/ra/fsp/src/r_dmac/r_dmac.c'
        original = source.read_text()
        with tempfile.TemporaryDirectory(prefix='k1-dmac-policy-') as temp:
            stage = Path(temp)
            path = stage / 'ra/fsp/src/r_dmac/r_dmac.c'
            path.parent.mkdir(parents=True)
            path.write_text(original)
            receipt = builder.stage_dmac_control_initialisation(stage)
            patched = path.read_text()
            self.assertNotEqual(receipt['before_sha256'], receipt['after_sha256'])
            with self.assertRaises(RuntimeError):
                builder.stage_dmac_control_initialisation(stage)
            for priority in (0, 1):
                for label, text in (('patched', patched), ('old', original)):
                    start = text.index('fsp_err_t R_DMAC_Open (')
                    end = text.index('\n}\n', start) + 3
                    fixture = stage / f'{label}-{priority}.c'
                    fixture.write_text(PREAMBLE + text[start:end] + MAIN)
                    binary = fixture.with_suffix('')
                    subprocess.run(['cc', '-std=c11', f'-DDMAC_CFG_PRIORITY_MODE={priority}',
                                    str(fixture), '-o', str(binary)], check=True)
                    ran = subprocess.run([str(binary)], cwd=stage, capture_output=True)
                    if label == 'patched':
                        self.assertEqual(ran.returncode, 0, ran.stderr)
                    else:
                        self.assertNotEqual(ran.returncode, 0,
                                            'old unsafe policy rewrite escaped the gate')


if __name__ == '__main__':
    unittest.main()
