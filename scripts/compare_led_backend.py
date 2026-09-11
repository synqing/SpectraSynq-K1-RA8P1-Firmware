#!/usr/bin/env python3
"""Compare SenseGlow-style GPIO-PODR cost against GPT duty-table cost."""
from __future__ import annotations

import tempfile
from pathlib import Path

from test_gold_extract import compile_and_run, parse
from verify_imports import ROOT  # noqa: F401  # identity of this tree


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="k1-led-backend-") as temp:
        fields = parse(compile_and_run(Path(temp) / "test").splitlines()[0])
        gpt = int(fields["gpt80_data_bytes"])
        podr = int(fields["podr80_bytes"])
        print(
            "K1_LED_BACKEND_COMPARE=PASS method=cost_by_lane_pixels "
            f"gpt80_data_bytes={gpt} gpt80_data_ns={fields['gpt80_data_ns']} "
            f"gpt80_reset_ns={fields['gpt80_reset_ns']} podr80_bytes={podr} "
            f"gpt160_data_bytes={fields['gpt160_data_bytes']} "
            f"gpt160_emit_ns={fields['gpt160_emit_ns']} "
            f"podr160_bytes={fields['podr160_bytes']} "
            f"double_buf_2x160={fields['double_buf_2x160']} winner=gpt_duty_table"
        )
        if podr <= gpt:
            raise SystemExit("PODR cost did not exceed GPT duty-table cost")


if __name__ == "__main__":
    main()
