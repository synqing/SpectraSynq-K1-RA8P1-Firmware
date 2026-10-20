"""Lane P4 step 7: the analysis-DTCM placement guard accepts exactly the declared objects."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
try:
    from build_scalar import verify_analysis_dtcm
finally:
    sys.path.pop(0)

SYMBOLS = "20000000 g .dtcm __dtcm_from_flash$$Base\n20010230 g .dtcm __dtcm_from_flash$$Limit\n"
GOOD = ("20000000 00005eb8 b (anonymous namespace)::palettes\n"
        "20005ec0 0000a370 b k1::titan::inhouseAudioDtcm()::state\n"
        "22000000 00000010 b elsewhere\n")


def test_exact_objects_pass():
    ev = verify_analysis_dtcm(SYMBOLS, GOOD)
    assert [o["name"] for o in ev["objects"]] == ["(anonymous namespace)::palettes",
                                                  "k1::titan::inhouseAudioDtcm()::state"]


def test_planted_extra_object_is_red():
    with pytest.raises(RuntimeError):
        verify_analysis_dtcm(SYMBOLS, GOOD + "20010000 00000100 b capture_poly24\n")


def test_planted_missing_object_is_red():
    with pytest.raises(RuntimeError):
        verify_analysis_dtcm(SYMBOLS, "20000000 00005eb8 b (anonymous namespace)::palettes\n")


def test_region_outside_dtcm_is_red():
    with pytest.raises(RuntimeError):
        verify_analysis_dtcm(SYMBOLS.replace("20010230", "20030000"), GOOD)
