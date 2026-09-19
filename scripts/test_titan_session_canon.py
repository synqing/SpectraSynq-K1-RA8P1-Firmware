#!/usr/bin/env python3
"""Regression for the evidence-bound Titan microphone/LED2 closeout canon."""
from __future__ import annotations

import json
import os
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = Path(os.environ.get(
    "K1_TITAN_EVIDENCE_ROOT",
    "/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002",
))
BUILD_ID = "541209cf0ac855af45c2793967f64e1e3a31a4e901cb6b0cbb02556fd5a2589c"
HEX_SHA = "c3bcea8a49c4461b51b7b8751bfcc5f6422430c65564c9a6381267550a0c55f7"
UID = "545433931bd25436593630352d068363"
SEQUENCE = [[0, 0], [1, 0], [0, 0], [0, 1], [0, 0], [1, 1], [0, 0]]


def load(relative: str) -> dict:
    return json.loads((EVIDENCE / relative).read_text())


def main() -> None:
    agents = (ROOT / "AGENTS.md").read_text()
    canon = (ROOT / "docs/evidence/K1-RA8P1-002/TITAN-MIC-LED2-SESSION-CANON.md").read_text()
    for required in (
        "Start the programmer waiter before any reply, then reply exactly `WAITING`",
        "do not repeat it",
        "TITAN-MIC-LED2-SESSION-CANON.md",
    ):
        assert required in agents, required
    assert "the chat is the operator instrument" in agents
    assert "as silent progress" not in agents
    assert "Stay silent; programming is in progress" not in canon
    for required in (
        "First clocked receive call is ACK",
        "PC11",
        "PC12",
        "PA07",
        "0x001CC916",
        "K1_LED2_PHY_STUB",
        "16 kHz is diagnostic only",
        "Completed button action was repeated",
    ):
        assert required in canon, required

    preflight = load("led2-mdio-diagnostic-preflight-02/receipt.json")
    programme = load("led2-mdio-diagnostic-prog-01/receipt.json")
    run = load("led2-mdio-diagnostic-run-01/receipt.json")
    optical = load("led2-mdio-session-canon-01/captain-optical-observation.json")
    microphone = load("exp-e-onboard-mic-e1e2.json")

    assert preflight["pass"] is True and preflight["programmed"] is False
    assert preflight["build"]["build_id"] == BUILD_ID
    assert preflight["build"]["hex_sha256"] == HEX_SHA
    assert programme["pass"] is True and programme["uid"] == UID
    assert programme["build_id"] == BUILD_ID and programme["image_sha256"] == HEX_SHA
    assert run["pass"] is True and run["CURRENT_TARGET"] is True
    assert run["runtime"]["uid"] == UID and run["runtime"]["build"] == BUILD_ID
    assert run["ready_status"]["led2"]["id1"] == 0x001C
    assert run["ready_status"]["led2"]["id2"] == 0xC916
    assert run["ready_status"]["led2"]["addr"] == 1
    assert run["ready_status"]["led2"]["reg_readback"] is True
    assert run["observed_channels"] == SEQUENCE
    assert run["optical"] == "NOT_CLAIMED"
    assert optical["evidence_class"] == "OPERATOR_REPORTED_OPTICAL"
    assert optical["build_id"] == BUILD_ID and optical["uid"] == UID
    assert optical["instrument_capture"] is False
    assert optical["states_reported_working"] == ["OFF", "GREEN", "YELLOW", "BOTH"]
    assert microphone["capture_profile"] == "diagnostic_16k_not_admitted_to_AP"
    assert microphone["physical_identity"]["acoustic_identity"] == "unproven"
    assert microphone["silicon_run"]["clipping"] == "both lanes reached int16 rails during diagnostic 16 kHz capture"
    print("K1_TITAN_SESSION_CANON=PASS")


if __name__ == "__main__":
    main()
