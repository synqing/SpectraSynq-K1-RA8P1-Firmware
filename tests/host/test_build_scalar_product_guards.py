"""Lane P2 iteration 2: product-image guards in scripts/build_scalar.py must go red.

* the soft-double guard sees calls AND tail calls (b, b.w, conditional) to every
  double-precision AEABI helper, ignores in-helper jumps (<helper+0x..>), and admits
  only the four 64-bit integer <-> double conversions FPv5-D16 cannot do in hardware;
* the real final P2 image (a8215f75) passes and its conversion counts are reported;
* --asrc-tcm data+code is refused without the explicit experiment flag.
"""
import importlib.util
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
# build_scalar imports its sibling verify_imports; scope the path change to this
# import so other test modules' collection is unaffected.
sys.path.insert(0, str(ROOT / "scripts"))
try:
    spec = importlib.util.spec_from_file_location("build_scalar", ROOT / "scripts/build_scalar.py")
    bs = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bs)
finally:
    sys.path.remove(str(ROOT / "scripts"))
    sys.modules.pop("verify_imports", None)

ATTRS = "Tag_MVE_arch: MVE Integer and Float\n"
MACROS = "#define __ARM_FEATURE_MVE 3\n#define __ARM_FP 14\n"
FINAL = ROOT / "_scratch/waveform-migration-001/arm-p2-final-product-008/disassembly.txt"


def line(op, target):
    return f" 2099000:\tf000 b80a \t{op}\t2099100 <{target}>\n"


@pytest.mark.parametrize("op", ["bl", "blx", "b", "b.w", "b.n", "beq.w", "bne"])
@pytest.mark.parametrize("helper", ["__aeabi_dmul", "__aeabi_dadd", "__aeabi_ddiv", "__aeabi_dcmplt",
                                    "__aeabi_d2f", "__aeabi_i2d", "__aeabi_ui2d", "__aeabi_f2d"])
def test_planted_soft_double_goes_red(op, helper):
    with pytest.raises(AssertionError, match="software double-precision helpers"):
        bs.assert_product_generated_code(line(op, helper), ATTRS, MACROS)


@pytest.mark.parametrize("helper", sorted(bs.INT64_CONVERSIONS))
def test_int64_conversions_admitted_and_counted(helper):
    dump = line("bl", helper) + line("b.w", helper)
    assert bs.assert_product_generated_code(dump, ATTRS, MACROS) == {helper: 2}


def test_in_helper_jump_is_not_a_call():
    assert bs.assert_product_generated_code(line("b.n", "__aeabi_dmul+0x16"), ATTRS, MACROS) == {}


@pytest.mark.skipif(not FINAL.exists(), reason="final P2 build output not present")
def test_final_p2_image_passes_with_tail_calls_counted():
    counts = bs.assert_product_generated_code(FINAL.read_text(), ATTRS, MACROS)
    assert set(counts) <= bs.INT64_CONVERSIONS
    assert counts.get("__aeabi_d2ulz", 0) >= 1  # the b.w tail call the old guard missed


def test_old_guard_missed_the_tail_call():
    old = __import__("re").compile(r'\bbl\s+[0-9a-f]+ <(__aeabi_d\w+)>')
    assert old.findall(line("b.w", "__aeabi_dmul")) == []
    assert bs.SOFT_DOUBLE_CALL.findall(line("b.w", "__aeabi_dmul")) == ["__aeabi_dmul"]


def test_itcm_data_code_refused_without_experiment_flag(tmp_path):
    run = subprocess.run([sys.executable, str(ROOT / "scripts/build_scalar.py"), "--output", str(tmp_path / "x"),
                          "--asrc-tcm", "data+code"], capture_output=True, text=True)
    assert run.returncode == 2
    assert "did not boot" in run.stderr
