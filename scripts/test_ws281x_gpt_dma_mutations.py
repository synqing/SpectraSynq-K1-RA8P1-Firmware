#!/usr/bin/env python3
"""Prove actual driver tests reject sequencing and false-completion regressions."""
from pathlib import Path
import os
import shutil
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
source = ROOT / "platform/ra8p1/ws281x_gpt_dma_hw.c"
mutations = {
    "missing_elc_module_start": (
        "    R_BSP_MODULE_START(FSP_IP_ELC, 0);",
        "    /* module-start deliberately omitted */",
    ),
    "lost_first_pulse": (
        "    gpt6_ctrl.p_reg->GTIOR |= 1u << 4;",
        "    /* first-pulse initialization deliberately omitted */",
    ),
    "frame_before_dma_irq": (
        "        if (dma_complete && waveform_complete &&",
        "        if (waveform_complete &&",
    ),
    "incomplete_dma_waiver": (
        "    if (dmac_ctrl.p_reg->DMCRA != 0u) {\n",
        "    if (dmac_ctrl.p_reg->DMCRA > 2u) {\n",
    ),
    "wrong_fault_frame": (
        "    fault_witness.frame_id = attempts;",
        "    fault_witness.frame_id = attempts - 1u;",
    ),
}


def _run_mutated_binary(work: Path) -> tuple[int, str]:
    """Compile and run the host GPT driver test against the mutated tree.

    Avoid nesting through test_ws281x_gpt_dma_hw.py: after a deliberate
    abort(), that wrapper's wait can stall on Darwin. Drive cc + binary
    here with a hard process-group kill so the assertion text is decisive.
    """
    exe = work / "gpt-hw-mutation"
    compile_cmd = [
        "cc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-I" + str(work / "tests/target_mock"),
        "-I" + str(work / "platform/ra8p1"),
        str(work / "tests/test_ws281x_gpt_dma_hw.c"),
        str(work / "platform/ra8p1/ws281x_waveform.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(compile_cmd, check=True, capture_output=True, text=True, timeout=120)
    proc = subprocess.Popen(
        [str(exe)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        start_new_session=True,
    )
    try:
        stdout, stderr = proc.communicate(timeout=8)
    except subprocess.TimeoutExpired as exc:
        # Darwin can leave an abort() child un-reapable briefly; do not wait
        # again. Assertion text already on the pipe is enough to score REJECTED.
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
        err = exc.stderr or b""
        out = exc.stdout or b""
        if isinstance(err, bytes):
            err = err.decode("utf-8", "replace")
        if isinstance(out, bytes):
            out = out.decode("utf-8", "replace")
        return 124, err + out
    return proc.returncode or 0, (stderr or "") + (stdout or "")

with tempfile.TemporaryDirectory() as tmp:
    work = Path(tmp)
    for sub in ["platform/ra8p1", "tests/target_mock", "tests"]:
        (work / sub).mkdir(parents=True, exist_ok=True)
    for name in [
        "ws281x_gpt_dma_hw.h",
        "ws281x_gpt_dma.h",
        "ws281x_waveform.c",
        "ws281x_waveform.h",
        "ws281x_diag.h",
        "titan_led_pins.h",
    ]:
        shutil.copy2(ROOT / "platform/ra8p1" / name, work / "platform/ra8p1" / name)
    shutil.copy2(ROOT / "tests/target_mock/ws281x_hw_mock.h", work / "tests/target_mock/ws281x_hw_mock.h")
    shutil.copy2(ROOT / "tests/test_ws281x_gpt_dma_hw.c", work / "tests/test_ws281x_gpt_dma_hw.c")

    for name, (old, new) in mutations.items():
        text = source.read_text()
        assert text.count(old) == 1, name
        (work / "platform/ra8p1/ws281x_gpt_dma_hw.c").write_text(text.replace(old, new))
        t0 = time.time()
        rc, err = _run_mutated_binary(work)
        elapsed = round(time.time() - t0, 2)
        rejected = rc != 0 and ("Assertion" in err or "assertion" in err)
        if not rejected:
            print(
                f"{name}: NOT_REJECTED rc={rc} elapsed={elapsed} err={err[-500:]!r}",
                flush=True,
            )
            raise SystemExit(f"K1_GPT_MUTATION_PROOF=FAIL mutation={name}")
        print(f"{name}: REJECTED elapsed={elapsed}", flush=True)

print("K1_GPT_MUTATION_PROOF=PASS")
