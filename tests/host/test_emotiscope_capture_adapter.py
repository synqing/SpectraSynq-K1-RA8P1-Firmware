#!/usr/bin/env python3
"""Execute production direct-PDM-to-Emotiscope adapter; no target access."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
P = ROOT / "platform/ra8p1"
spec = importlib.util.spec_from_file_location(
    "legacy_admission", ROOT / "tests/host/test_pdm_asrc_admission.py")
legacy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(legacy)

def capture_harness(source):
    old_cases = (ROOT/"tests/host/test_pdm_asrc_admission_cases.c").read_text()
    prefix = legacy.harness(source).split("/* Appended by test_pdm_asrc_admission.py")[0]
    helpers = "\n".join(legacy.function(old_cases, name) for name in (
        "initialise_host", "raw_pair", "rate_ignores_downstream_admission",
        "metadata_loss_requires_discontinuity"))
    return prefix + helpers + "\n" + (
        ROOT/"tests/host/test_emotiscope_capture_admission_cases.c").read_text()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--output", required=True, type=Path)
    args = ap.parse_args()
    source = (P/"pdm_target.c").read_text()
    adapter = (P/"k1_asrc_24k.c").read_text()
    quality = (ROOT/"tests/host/test_emotiscope_capture_adapter.c").read_text()
    variants = [
        ("adapter_actual", quality, adapter, False),
        ("admission_actual", capture_harness(source), adapter, False),
        ("reject_unfiltered_aliasing", quality,
         adapter.replace("st->ring[st->write_pos] = sat16((int32_t)\n"
                         "            (filtered + (filtered >= 0.0f ? 0.5f : -0.5f)));",
                         "(void)filtered; st->ring[st->write_pos] = src[i];"), True),
        ("reject_legacy_contract", capture_harness(source.replace(
            "out->contract_version = K1_PDM_TARGET_AUDIO_CONTRACT_VERSION;",
            "out->contract_version = 1u;")), adapter, True),
        ("reject_missing_filter_delay", capture_harness(source.replace(
            "delta_q16 += (uint64_t)K1_ASRC24_FILTER_DELAY_SAMPLES * 65536ull;",
            "/* mutation: omit FIR delay */")), adapter, True),
        ("reject_hidden_audio_loss", capture_harness(source.replace(
            "capture_discontinuity_pending = reason != 0u ? 1u : 0u;",
            "capture_discontinuity_pending = 0u;")), adapter, True),
        ("reject_lost_queue_witness", capture_harness(source.replace(
            "capture_service.reject_filled = capture_asrc.filled;",
            "capture_service.reject_filled = 0u;")), adapter, True),
        ("reject_wrong_poll_index", capture_harness(source.replace(
            "capture_service.reject_poll_pair_index = capture_poll_pairs + 1u;",
            "capture_service.reject_poll_pair_index = 1u;")), adapter, True),
    ]
    assert variants[2][2] != adapter
    results = []
    with tempfile.TemporaryDirectory(prefix="emotiscope-capture-") as temporary:
        td = Path(temporary)
        for name, test_source, adapter_source, negative in variants:
            test = td/(name+".c")
            asrc = td/(name+"_asrc.c")
            binary = td/name
            test.write_text(test_source)
            asrc.write_text(adapter_source)
            command = ["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                       "-Werror", "-Wno-unused-variable", "-Wno-unused-function",
                       "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                       "-DK1_EMOTISCOPE_PORT", "-DK1_PDM_STREAM_SLOT_COUNT=4u",
                       "-I"+str(P), "-I"+str(ROOT/"src"), str(test), str(asrc),
                       str(P/"pdm_capture.c"),
                       str(ROOT/"src/k1/core/audio/k1_audio_hop.c"),
                       "-lm", "-o", str(binary)]
            built = subprocess.run(command, capture_output=True, text=True)
            if built.returncode:
                raise RuntimeError(built.stderr)
            ran = subprocess.run([str(binary)], capture_output=True, text=True)
            passed = (ran.returncode != 0 and "Assertion failed" in ran.stderr) if negative else ran.returncode == 0
            results.append({"case":name,"pass":passed,"negative":negative,
                            "exit":ran.returncode,"stdout":ran.stdout,
                            "stderr":ran.stderr,"compile_command":command})
            print(name, "PASS" if passed else "FAIL", "exit", ran.returncode)
            if not passed:
                print(ran.stdout, ran.stderr)
        paths = [P/"k1_asrc_24k.c",P/"k1_asrc_24k.h",P/"pdm_target.c",
                 P/"pdm_target.h",Path(__file__).resolve(),
                 ROOT/"tests/host/test_emotiscope_capture_adapter.c",
                 ROOT/"tests/host/test_emotiscope_capture_admission_cases.c"]
        receipt = {"pass":all(r["pass"] for r in results),
                   "scope":"Host production adapter and admission; no target timing or acoustic calibration",
                   "rate_hz":12800,"samples":64,"contract_version":2,
                   "fir_taps":95,"fir_delay_source_samples":47,
                   "gain":"Existing nominal16x PCM loud guard remains before this adapter",
                   "quality_limits":"55--4000Hz sampled tones: -0.6..+0.1dB; 6400--17000Hz sampled tones: below -45dB; source36/40/44k",
                   "sources":{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                   "results":results}
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(receipt, indent=2)+"\n")
        assert receipt["pass"]

if __name__ == "__main__":
    main()
