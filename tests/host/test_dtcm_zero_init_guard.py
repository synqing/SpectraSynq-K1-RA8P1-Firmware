"""Lane P4 lesson: initialised data in fsp.ld's NOLOAD .dtcm is silently zeroed on the device.
The build guard must accept zero-filled .dtcm and refuse non-zero bytes or relocations."""
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
try:
    from build_scalar import verify_dtcm_zero_init
finally:
    sys.path.pop(0)

TC = ROOT.parents[2] / "toolchains/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin"
GCC, READELF = TC / "arm-none-eabi-gcc", TC / "arm-none-eabi-readelf"
pytestmark = pytest.mark.skipif(not GCC.exists(), reason="ARM toolchain not installed")


def _obj(tmp, name, source):
    c = tmp / f"{name}.c"
    c.write_text(source)
    o = tmp / f"{name}.o"
    subprocess.run([str(GCC), "-c", "-O2", str(c), "-o", str(o)], check=True)
    return o


def _readelf(path, *args):
    return subprocess.run([str(READELF), *args, str(path)], capture_output=True, text=True, check=True).stdout


def test_zero_filled_scratch_passes(tmp_path):
    o = _obj(tmp_path, "ok", 'float scratch[64] __attribute__((section(".dtcm")));\n')
    assert verify_dtcm_zero_init([o], _readelf)["objects_with_dtcm"] == 1


def test_flash_initialised_section_is_ignored(tmp_path):
    o = _obj(tmp_path, "flash", 'int state __attribute__((section(".dtcm_from_flash"))) = 5;\n')
    assert verify_dtcm_zero_init([o], _readelf)["objects_with_dtcm"] == 0


def test_planted_initialiser_is_red(tmp_path):
    o = _obj(tmp_path, "bad", 'int zero __attribute__((section(".dtcm")));\n'
                              'int bad __attribute__((section(".dtcm"))) = 5;\n')
    with pytest.raises(RuntimeError, match="non-zero"):
        verify_dtcm_zero_init([o], _readelf)


def test_planted_pointer_initialiser_is_red(tmp_path):
    o = _obj(tmp_path, "ptr", 'extern int target;\nint *p __attribute__((section(".dtcm"))) = &target;\n')
    with pytest.raises(RuntimeError, match="relocation"):
        verify_dtcm_zero_init([o], _readelf)
