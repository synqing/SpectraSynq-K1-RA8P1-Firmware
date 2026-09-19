#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
from source_ownership import (  # noqa: E402
    process_source_ok,
    should_leave_session_alone,
    titan_project_ok,
)

REPLAY = HERE / "titan_fixture_replay.py"


def test_uart_project_is_not_titan_process_io():
    io = {"isConnected": True, "busType": 0, "busTypeSlug": "uart"}
    sources = [{"busType": 0, "busTypeSlug": "uart", "connection": {"portName": "cu.usbmodem1"}}]
    ok, reason = process_source_ok(io, sources, REPLAY)
    assert ok is False
    assert "Process I/O" in reason
    leave = should_leave_session_alone(
        io, {"title": "K1 Dual UART Observability v2", "groupCount": 15}, sources, REPLAY
    )
    assert leave is not None


def test_title_and_group_count_alone_are_not_enough():
    io = {"isConnected": False, "busType": 0, "busTypeSlug": "uart"}
    sources = [{"busType": 0, "connection": {}}]
    proj = {"title": "Titan Mini Observability", "groupCount": 12}
    assert titan_project_ok(proj)[0] is True
    assert process_source_ok(io, sources, REPLAY)[0] is False


def test_proof_must_not_touch_a_live_uart_session():
    io = {"isConnected": True, "busType": 0, "busTypeSlug": "uart"}
    sources = [{"busType": 0, "busTypeSlug": "uart"}]
    leave = should_leave_session_alone(
        io, {"title": "Titan Mini Observability", "groupCount": 12}, sources, REPLAY
    )
    assert leave is not None
    assert "UART" in leave or "Process I/O" in leave


def test_matching_process_io_is_ok():
    io = {"isConnected": True, "busType": 8, "busTypeSlug": "process"}
    sources = [{"busType": 8, "busTypeSlug": "process"}]
    proc = {"executable": str(REPLAY.resolve()), "mode": 0}
    ok, reason = process_source_ok(io, sources, REPLAY, proc)
    assert ok is True, reason
    assert (
        should_leave_session_alone(
            io, {"title": "Titan Mini Observability", "groupCount": 12}, sources, REPLAY, proc
        )
        is None
    )


if __name__ == "__main__":
    test_uart_project_is_not_titan_process_io()
    test_title_and_group_count_alone_are_not_enough()
    test_proof_must_not_touch_a_live_uart_session()
    test_matching_process_io_is_ok()
    print("ownership_tests_ok")
