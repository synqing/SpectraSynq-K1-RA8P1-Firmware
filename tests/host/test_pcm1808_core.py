from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import shutil
import sys


ROOT = Path(__file__).resolve().parents[2]
DONOR = Path("/Users/spectrasynq/SpectraSynq_K1_Firmware")
PIN = "0b542364abeec1337bd74d7b3b9010e37b969401"
sys.path.insert(0, str(ROOT / "scripts"))
from build_scalar import BSP, stage_pcm1808_vectors  # noqa: E402


def test_pcm1808_donor_headers_are_exact_pinned_copies():
    pairs = {
        "k1_pcm1808_unpack.h": "SPECTRASYNQ_K1_FIRMWARE/audio/k1_pcm1808_unpack.h",
        "k1_pcm1808_ring.h": "SPECTRASYNQ_K1_FIRMWARE/audio/k1_pcm1808_ring.h",
        "k1_pcm1808_map.h": "SPECTRASYNQ_K1_FIRMWARE/audio/k1_pcm1808_map.h",
        "k1_pcm1808_resampler_coeffs.h": "SPECTRASYNQ_K1_FIRMWARE/audio/k1_pcm1808_resampler_coeffs.h",
        "k1_resample_48k_to_12k8.h": "SPECTRASYNQ_K1_FIRMWARE/audio/k1_resample_48k_to_12k8.h",
    }
    for local_name, donor_name in pairs.items():
        source = subprocess.check_output(["git", "-C", str(DONOR), "show", f"{PIN}:{donor_name}"])
        local = (ROOT / "platform/ra8p1/pcm1808" / local_name).read_bytes()
        assert hashlib.sha256(local).digest() == hashlib.sha256(source).digest()


def test_pcm1808_contract_names_complete_ssie1_route_and_open_gates():
    contract = json.loads((ROOT / "docs/pcm1808-source-contract.json").read_text())
    route = contract["titan_route"]
    assert route["peripheral"] == "SSIE1"
    assert route["role"] == "slave receiver"
    assert route["transfer"] == "DTC block receive with two 7.5 ms application buffers"
    assert {(item["mcu"], item["u11_pin"]) for item in route["signals"]} == {
        ("P702/SSIBCK1_B", 24),
        ("P701/SSILRCK1_B", 33),
        ("P700/SSIDATA1_B", 26),
    }
    assert route["u18_complete_ssie_route"] is False
    assert contract["admission"]["wired_titan"] == "NOT_RUN"
    boundary = contract["production_ap_boundary"]
    assert boundary["direct_connection_to_production_ap"] is False
    assert "sr24000.hop180" in boundary["required_contract"]


def test_pcm1808_core_compiles_and_runs(tmp_path):
    executable = tmp_path / "pcm1808_core"
    subprocess.check_call([
        "c++", "-std=c++17", "-O2",
        "-I", str(ROOT / "platform/ra8p1"),
        str(ROOT / "platform/ra8p1/pcm1808_core.cpp"),
        str(ROOT / "tests/host/test_pcm1808_core.cpp"),
        "-o", str(executable),
    ])
    subprocess.check_call([str(executable)])


def test_pcm1808_disposable_stage_allocates_ssie1_vectors(tmp_path):
    stage = tmp_path / "stage"
    (stage / "ra_gen").mkdir(parents=True)
    for name in ("vector_data.h", "vector_data.c"):
        shutil.copy2(BSP / "FSPConfiguration/ra_gen" / name, stage / "ra_gen" / name)
    stage_pcm1808_vectors(stage)
    header = (stage / "ra_gen/vector_data.h").read_text()
    source = (stage / "ra_gen/vector_data.c").read_text()
    assert "VECTOR_DATA_IRQ_COUNT    (76)" in header
    assert "PCM1808_SSI1_RXI_IRQn ((IRQn_Type) 74)" in header
    assert "PCM1808_SSI1_INT_IRQn ((IRQn_Type) 75)" in header
    assert "[74] = ssi_rxi_isr" in source
    assert "[75] = ssi_int_isr" in source
    assert "EVENT_SSI1_RXI" in source
    assert "EVENT_SSI1_INT" in source


def test_pcm1808_target_starts_after_usb_and_is_continuously_polled():
    entry = (ROOT / "platform/ra8p1/hal_entry.c").read_text()
    configured = entry.index("case USB_STATUS_CONFIGURED")
    initialise = entry.index("if(attached && !k1_pcm1808_target_initialised())")
    poll = entry.index("k1_pcm1808_target_poll();")
    assert configured < initialise < poll
    assert "K1_PCM1808_TARGET" in entry
    fixture = (ROOT / "platform/ra8p1/fixture_app.cpp").read_text()
    assert "command==15" in fixture
    assert "k1_pcm1808_target_metrics" in fixture
