#!/usr/bin/env python3
"""Host tests for e2lite_debug_workflow. Never opens hardware."""
from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import e2lite_debug_workflow as w  # noqa: E402


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        # Required negative: full-register path rejected
        out = root / "reject.json"
        rc = w.main(["reject-full-register", "--output", str(out), "--attach", "getAllRegisters"])
        data = json.loads(out.read_text())
        assert rc == 0 and data["pass"] is True and data["prohibited"] is True, data

        out2 = root / "reject-gdb.json"
        rc = w.main(["reject-full-register", "--output", str(out2), "--attach", "arm-none-eabi-gdb"])
        data = json.loads(out2.read_text())
        assert rc == 0 and data["pass"] is True, data

        # Identity command preparation (no execute)
        out3 = root / "sig.json"
        rc = w.main(["prepare-identity-command", "--output", str(out3)])
        data = json.loads(out3.read_text())
        assert rc == 0 and data["pass"] is True
        assert "-sig" in data["command"] and "-run" in data["command"]
        assert "getAllRegisters" not in json.dumps(data)

    print("K1_E2LITE_DEBUG_WORKFLOW_HOST=PASS (getAllRegisters rejected, sig command prepared)")


if __name__ == "__main__":
    main()
