#!/usr/bin/env python3
"""Reject shared PDM/LED DMAC channels; keep pdm-first owners 0/1/2/3."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def compile_guard(extra_defines, expect_fail):
    src = r'''
typedef int IRQn_Type;
#define DMAC0_INT_IRQn ((IRQn_Type) 14)
#define DMAC1_INT_IRQn ((IRQn_Type) 74)
#define DMAC2_INT_IRQn ((IRQn_Type) 76)
#define DMAC3_INT_IRQn ((IRQn_Type) 77)
#define VECTOR_NUMBER_DMAC0_INT ((IRQn_Type) 14)
#define VECTOR_NUMBER_DMAC1_INT ((IRQn_Type) 74)
#define VECTOR_NUMBER_DMAC2_INT ((IRQn_Type) 76)
#define VECTOR_NUMBER_DMAC3_INT ((IRQn_Type) 77)
#define K1_PALETTE_WS2816_GPT_PAIR 1
#define K1_PDM_TARGET 1
#include "k1_dmac_lane_guard.h"
int main(void) { return 0; }
'''
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / 'guard.c'
        exe = Path(temp) / 'guard'
        path.write_text(src)
        cmd = [
            'cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
            '-I' + str(ROOT / 'tests/target_mock'),
            '-I' + str(ROOT / 'platform/ra8p1'),
            *extra_defines,
            str(path), '-o', str(exe),
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        failed = result.returncode != 0
        if expect_fail != failed:
            raise SystemExit(
                f'expected_fail={expect_fail} got rc={result.returncode}\n'
                f'{result.stderr}'
            )
        if expect_fail:
            blob = result.stderr + result.stdout
            if 'share a DMAC channel' not in blob:
                raise SystemExit('collision compile failed without the guard text:\n' + blob)


def test_collision_is_rejected():
    compile_guard([
        '-DK1_WS281X_GPT_DMA_CHANNEL=0u',
        '-DK1_WS281X_GPT_DMA_IRQ=DMAC0_INT_IRQn',
        '-DK1_WS281X_GPT_DMA_CHANNEL_A1=3u',
        '-DK1_WS281X_GPT_DMA_IRQ_A1=DMAC3_INT_IRQn',
        '-DK1_PDM_TARGET_RISE_DMA_CHANNEL=0u',
        '-DK1_PDM_TARGET_RISE_DMA_IRQ=DMAC0_INT_IRQn',
        '-DK1_PDM_TARGET_FALL_DMA_CHANNEL=1u',
        '-DK1_PDM_TARGET_FALL_DMA_IRQ=DMAC1_INT_IRQn',
    ], expect_fail=True)


def test_pdm_first_four_owners_compile():
    compile_guard([
        '-DK1_WS281X_GPT_DMA_CHANNEL=2u',
        '-DK1_WS281X_GPT_DMA_IRQ=DMAC2_INT_IRQn',
        '-DK1_WS281X_GPT_DMA_CHANNEL_A1=3u',
        '-DK1_WS281X_GPT_DMA_IRQ_A1=DMAC3_INT_IRQn',
        '-DK1_PDM_TARGET_RISE_DMA_CHANNEL=0u',
        '-DK1_PDM_TARGET_RISE_DMA_IRQ=DMAC0_INT_IRQn',
        '-DK1_PDM_TARGET_FALL_DMA_CHANNEL=1u',
        '-DK1_PDM_TARGET_FALL_DMA_IRQ=DMAC1_INT_IRQn',
    ], expect_fail=False)


def test_build_script_injects_pdm_first_and_led_first():
    build = (ROOT / 'scripts/build_scalar.py').read_text(encoding='utf-8')
    assert "'-DK1_WS281X_GPT_DMA_CHANNEL=2u'" in build
    assert "'-DK1_WS281X_GPT_DMA_IRQ=DMAC2_INT_IRQn'" in build
    assert "'-DK1_WS281X_GPT_DMA_CHANNEL=0u'" in build
    assert "'-DK1_PDM_TARGET_RISE_DMA_CHANNEL=0u'" in build
    assert "'-DK1_PDM_TARGET_FALL_DMA_CHANNEL=1u'" in build
    assert 'DMAC channel collision in compiler defines' in build


def test_pair_header_defaults_are_pdm_first():
    header = (ROOT / 'platform/ra8p1/ws281x_gpt_dma_hw_pair.h').read_text(encoding='utf-8')
    assert '#define K1_WS281X_GPT_DMA_CHANNEL 2u' in header
    assert '#define K1_WS281X_GPT_DMA_IRQ DMAC2_INT_IRQn' in header
    assert '#define K1_WS281X_GPT_DMA_CHANNEL_A1 3u' in header


if __name__ == '__main__':
    test_collision_is_rejected()
    test_pdm_first_four_owners_compile()
    test_build_script_injects_pdm_first_and_led_first()
    test_pair_header_defaults_are_pdm_first()
    print('DMAC_LANE_GUARD=PASS')
