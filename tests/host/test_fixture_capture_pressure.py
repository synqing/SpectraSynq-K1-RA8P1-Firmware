#!/usr/bin/env python3
"""Replay the observed queue boundary through production capture and owner code.

The prefix constructs a valid queued stream; it is not a reconstruction of the
unrecorded pre-fault schedule. DSP/render execution times and the due-frame phase
are explicit host seams. No hardware, playback or firmware mutation occurs.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
P = ROOT / "platform/ra8p1"
spec = importlib.util.spec_from_file_location(
    "capture_admission", ROOT / "tests/host/test_pdm_asrc_admission.py")
admission = importlib.util.module_from_spec(spec)
spec.loader.exec_module(admission)

C_SEAM = r'''
void pressure_reset(void) {
    initialise_host();
    k1_pdm_target_set_pcm_drain(0);
    assert(k1_asrc24_set_rate(&capture_asrc, 41417u) == k1_asrc24_ok);
    capture_rate_locked = 1u; /* Replay an already measured steady-state rate. */
    capture_measured_hz = 41417u;
}
void pressure_time(uint64_t now) { host_time_us = now; }
void pressure_raw(unsigned sequence, uint64_t receipt_us) {
    host_time_us = receipt_us - 7400u;
    raw_pair(sequence);
}
void pressure_poll(void) { k1_pdm_target_poll(); }
int pressure_pull(k1_audio_hop_t *hop) {
    return k1_pdm_target_try_read_ap_hop(hop);
}
unsigned pressure_filled(void) { return capture_asrc.filled; }
unsigned pressure_rejected(void) { return capture_asrc.push_rejected; }
unsigned pressure_discarded(void) { return capture_asrc.discarded_samples; }
unsigned pressure_consumed(void) { return capture_asrc.consumed_samples; }
unsigned pressure_raw_faults(void) {
    unsigned sum = 0;
    for (unsigned i = 0; i < 2; ++i)
        sum += capture_stream[i].overflow_events + capture_stream[i].drop_events
             + capture_stream[i].recovery_count;
    return sum;
}
k1_pdm_capture_service_t pressure_service(void) { return capture_service; }
'''


def owner_loop(source):
    begin = source.index("for (unsigned catchup = 0; catchup < 3U; ++catchup) {")
    end = source.index("    live_runtime.setCaptureMeta", begin)
    loop = "\n".join(line for line in source[begin:end].splitlines()
                     if not line.lstrip().startswith("#"))
    render_line = "if (palettes.frameDue(live_runtime.nowUs())) palette_step(true);"
    assert loop.count(render_line) == 1
    # Restore only the installed defect for the negative control. The repaired
    # variant below is extracted verbatim from the production owner.
    old_line = "if (palettes.frameDue(live_runtime.nowUs())) break;"
    assert old_line not in loop
    return loop, loop.replace(render_line, old_line)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    capture = (P / "pdm_target.c").read_text()
    owner = (P / "fixture_app.cpp").read_text()
    entry = (P / "hal_entry.c").read_text()
    start = entry.index("void hal_entry(void)")
    assert entry.index("k1_pdm_target_poll();", start) < entry.index("k1_fixture_poll(", start)
    candidate, old_loop = owner_loop(owner)
    begin = owner.index("static void fixture_make_pcm_room() {")
    end = owner.index("\n}\n#endif\nextern", begin) + 2
    drain = owner[begin:end]
    assert "k1_pdm_target_set_pcm_drain(&fixture_make_pcm_room);" in owner
    cases = (ROOT / "tests/host/test_pdm_asrc_admission_cases.c").read_text()
    capture_unit = admission.harness(capture).split(
        "/* Appended by test_pdm_asrc_admission.py")[0]
    capture_unit += "\n" + "\n".join(admission.function(cases, name)
                                     for name in ("initialise_host", "raw_pair"))
    capture_unit += C_SEAM
    fixture = (ROOT / "tests/host/test_fixture_capture_pressure.cpp").read_text()
    assert fixture.count("// INSERT_CURRENT_OWNER_LOOP") == 1
    assert fixture.count("// INSERT_INTERLEAVED_OWNER_LOOP") == 1
    assert fixture.count("// INSERT_PRODUCTION_PCM_DRAIN") == 1
    fixture = fixture.replace("// INSERT_CURRENT_OWNER_LOOP", old_loop).replace(
        "// INSERT_INTERLEAVED_OWNER_LOOP", candidate)
    fixture = fixture.replace("// INSERT_PRODUCTION_PCM_DRAIN", drain)
    results = []
    with tempfile.TemporaryDirectory(prefix="titan-capture-pressure-") as temporary:
        td = Path(temporary)
        (td / "capture.c").write_text(capture_unit)
        (td / "fixture.cpp").write_text(fixture)
        flags = ["-O1", "-g", "-Wall", "-Wextra", "-Werror",
                 "-Wno-unused-function", "-Wno-unused-variable",
                 "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                 "-DK1_EMOTISCOPE_PORT", "-DK1_PDM_STREAM_SLOT_COUNT=4u",
                 "-I" + str(P), "-I" + str(ROOT / "src")]
        objects = []
        sources = [td / "capture.c", P / "k1_asrc_24k.c", P / "pdm_capture.c",
                   ROOT / "src/k1/core/audio/k1_audio_hop.c"]
        commands = []
        for index, source in enumerate(sources):
            obj = td / f"unit{index}.o"
            command = ["cc", "-std=c11", *flags, "-c", str(source), "-o", str(obj)]
            subprocess.run(command, check=True, capture_output=True, text=True)
            commands.append(command)
            objects.append(str(obj))
        binary = td / "pressure"
        command = ["c++", "-std=c++17", *flags, str(td / "fixture.cpp"),
                   *objects, "-o", str(binary)]
        subprocess.run(command, check=True, capture_output=True, text=True)
        commands.append(command)
        for variant in ("observed", "interleaved", "require-lossless-current",
                        "third-pair-old", "third-pair-drain", "require-lossless-third-pair"):
            ran = subprocess.run([str(binary), variant], capture_output=True, text=True)
            expected_failure = variant in ("require-lossless-current", "require-lossless-third-pair")
            accepted = (ran.returncode != 0 and "pressure_rejected() == 0u" in ran.stderr
                        if expected_failure else ran.returncode == 0)
            results.append({"case": variant, "exit": ran.returncode,
                            "expected_failure": expected_failure, "accepted": accepted,
                            "stdout": ran.stdout, "stderr": ran.stderr})
            print(variant, "matched expected outcome" if accepted else "FAILED",
                  "exit", ran.returncode)
            print(ran.stdout, end="")
            if not accepted:
                print(ran.stderr, file=sys.stderr)
        source_paths = [Path(__file__).resolve(),
                        ROOT / "tests/host/test_fixture_capture_pressure.cpp",
                        ROOT / "tests/host/test_pdm_asrc_admission.py",
                        ROOT / "tests/host/test_pdm_asrc_admission_cases.c",
                        P / "pdm_target.c", P / "pdm_target.h", P / "pdm_capture.c",
                        P / "pdm_capture.h", P / "k1_asrc_24k.c", P / "k1_asrc_24k.h",
                        P / "fixture_app.cpp", P / "hal_entry.c"]
        record = {"scope": "Conditional host scheduling reproduction; no physical timing proof",
                  "source_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                    for p in source_paths},
                  "mocked": ["DSP cost 2500us", "render cost 1500us and due clock",
                             "palette_step render/emit boundary", "feature-demand setters",
                             "pair progress call", "DMA completion injection",
                             "two-pair poll wall cost 4468us", "constructed valid prefix"],
                  "not_established": ["actual pre-fault scheduling history",
                                      "USB causal attribution", "Sonic versus BOOM causal difference",
                                      "sustainable target load or physical correction"],
                  "commands": commands, "results": results}
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(record, indent=2) + "\n")
        assert all(r["accepted"] for r in results)


if __name__ == "__main__":
    main()
